#include "engine/flowgraph.hpp"

#include <array>
#include "logics/util.hpp"
#include "engine/util.hpp"
#include "util/log.hpp"
#include "util/timer.hpp"

using namespace plankton;

constexpr bool POSTPROCESS_FLOW_GRAPHS = false;
constexpr std::array<EMode, 2> AllEMode = {EMode::PRE, EMode::POST };

struct UpdateMap {
    using update_list_t = std::vector<std::pair<std::string, const SymbolDeclaration*>>;
    std::map<const SymbolDeclaration*, update_list_t> lookup;
    
    inline void Reset() {
        lookup.clear();
    }
    
    inline void Add(const SymbolDeclaration& address, const std::string& field, const SymbolDeclaration& value) {
        lookup[&address].emplace_back(field, &value);
    }
    
    [[nodiscard]] inline const update_list_t& GetUpdates(const SymbolDeclaration& address) const {
        assert(address.type.sort == Sort::PTR);
        static update_list_t empty = {};
        auto find = lookup.find(&address);
        if (find != lookup.end()) return find->second;
        else return empty;
    }
};

struct Worklist {
    std::set<std::pair<std::size_t, FlowGraphNode*>, std::greater<>> container;
    
    explicit Worklist() = default;
    explicit Worklist(std::size_t size, FlowGraphNode& node) { Add(size, node); }
    
    [[nodiscard]] inline bool Empty() const { return container.empty(); }
    
    inline bool Add(std::size_t size, FlowGraphNode& node) {
        return container.emplace(size, &node).second;
    }
    
    inline std::pair<std::size_t, FlowGraphNode*> Take() {
        auto element = *container.begin();
        container.erase(container.begin());
        return element;
    }
};

inline FlowGraphNode MakeNodeFromResource(const MemoryAxiom& axiom, SymbolFactory& factory, const FlowGraph& graph) {
    FlowGraphNode result(graph, axiom.node->Decl(), plankton::IsLocal(axiom), axiom.flow->Decl(), factory);
    
    auto& flowType = axiom.flow->Type();
    result.dataFields.reserve(axiom.fieldToValue.size());
    result.pointerFields.reserve(axiom.fieldToValue.size());
    for (const auto& [name, value] : axiom.fieldToValue) {
        if (value->Sort() == Sort::PTR) {
            result.pointerFields.emplace_back(name, value->Type(), value->Decl(), factory, flowType);
        } else {
            result.dataFields.emplace_back(name, value->Type(), value->Decl());
        }
    }
    
    return result;
}

inline std::unique_ptr<StackAxiom> MakeEquality(const SymbolDeclaration& lhs, std::unique_ptr<SymbolicExpression> rhs) {
    return std::make_unique<StackAxiom>(BinaryOperator::EQ, std::make_unique<SymbolicVariable>(lhs), std::move(rhs));
}

struct UpdateHelper {
    const SymbolDeclaration &trueValue, &falseValue, &minValue, &maxValue, &nullValue;
    SeparatingConjunction valuation;
    
    explicit UpdateHelper(SymbolFactory& factory)
            : trueValue(factory.GetFreshFO(Type::Bool())), falseValue(factory.GetFreshFO(Type::Bool())),
              minValue(factory.GetFreshFO(Type::Data())), maxValue(factory.GetFreshFO(Type::Data())),
              nullValue(factory.GetFreshFO(Type::Null())) {
        valuation.Conjoin(MakeEquality(trueValue, std::make_unique<SymbolicBool>(true)));
        valuation.Conjoin(MakeEquality(falseValue, std::make_unique<SymbolicBool>(false)));
        valuation.Conjoin(MakeEquality(minValue, std::make_unique<SymbolicMin>()));
        valuation.Conjoin(MakeEquality(maxValue, std::make_unique<SymbolicMax>()));
        valuation.Conjoin(MakeEquality(nullValue, std::make_unique<SymbolicNull>()));
    }
};

struct SymbolMaker : public BaseProgramVisitor {
    const Formula& state;
    const UpdateHelper& helper;
    const SymbolDeclaration* result = nullptr;
    explicit SymbolMaker(const Formula& state, const UpdateHelper& helper) : state(state), helper(helper) {}
    void Visit(const TrueValue& /*object*/) override { result = &helper.trueValue; }
    void Visit(const FalseValue& /*object*/) override { result = &helper.falseValue; }
    void Visit(const MinValue& /*object*/) override { result = &helper.minValue; }
    void Visit(const MaxValue& /*object*/) override { result = &helper.maxValue; }
    void Visit(const NullValue& /*object*/) override { result = &helper.nullValue; }
    void Visit(const VariableExpression& object) override { result = &plankton::Evaluate(object, state); }
    inline const SymbolDeclaration& AsSymbol(const SimpleExpression& expr) {
        result = nullptr;
        expr.Accept(*this);
        if (result) return *result;
        throw std::logic_error("Internal error: could not create logic representation for memory update.");
    }
};

struct FlowGraphGenerator {
    const MemoryWrite& command;
    FlowGraph& graph;
    SeparatingConjunction& state;
    SymbolFactory factory;
    Encoding encoding;
    UpdateHelper helper;
    UpdateMap updates;
    
    explicit FlowGraphGenerator(FlowGraph& empty, const MemoryWrite& command)
            : command(command), graph(empty), state(*empty.pre->now), factory(*graph.pre), helper(factory) {
        assert(command.lhs.size() == command.rhs.size());
    }
    
    void MakeUpdates() {
        updates.Reset();
        state.Conjoin(plankton::Copy(helper.valuation));
        SymbolMaker symbolMaker(state, helper);
        for (std::size_t index = 0; index < command.lhs.size(); ++index) {
            auto& dereference = *command.lhs.at(index);
            auto& address = symbolMaker.AsSymbol(*dereference.variable);
            auto& value = symbolMaker.AsSymbol(*command.rhs.at(index));
            updates.Add(address, dereference.fieldName, value);
        }
    }
    
    inline bool IsDistinct(const SymbolDeclaration& address) {
        std::vector<EExpr> vector;
        vector.reserve(graph.nodes.size());
        auto expr = encoding.Encode(address);
        for (const auto& node : graph.nodes) vector.push_back(encoding.Encode(node.address) != expr);
        return encoding.Implies(encoding.MakeAnd(vector));
    }
    
    inline void ApplyUpdates(FlowGraphNode& node) const {
        std::set<std::string> updatedFields;
        auto checkUpdate = [&updatedFields](const std::string& field) {
            auto insertion = updatedFields.insert(field);
            if (insertion.second) return;
            throw std::logic_error("Unsupported assignment: updates field '" + field + "' twice."); // TODO: better error handling
        };

        for (const auto& [fieldName, newValue] : updates.GetUpdates(node.address)) {
            checkUpdate(fieldName);
            auto& field = node.GetField(fieldName);
            field.postValue = *newValue;
        }
    }
    
    FlowGraphNode* TryGetOrCreateNode(const SymbolDeclaration& nextAddress) {
        // find the memory resource referenced by nextAddress, abort if none exists
        auto* resource = plankton::TryGetResource(nextAddress, state);
        if (!resource) return nullptr;

        // search for an existing node at the given address, care for aliasing
        auto& alias = resource->node->Decl();
        if (auto find = graph.GetNodeOrNull(alias)) return find;
        if (!IsDistinct(alias)) return nullptr;

        // create a new node, add to footprint
        auto newNode = MakeNodeFromResource(*resource, factory, graph);
        ApplyUpdates(newNode);
        graph.nodes.push_back(std::move(newNode));
        return &graph.nodes.back();
    }

    inline void InlineAliases(FlowGraphNode& node) {
        auto getPointerValue = [&](const SymbolDeclaration& address) -> const SymbolDeclaration& {
            if (auto node = graph.GetNodeOrNull(address)) return node->address;
            if (auto memory = plankton::TryGetResource(address, state)) return memory->node->Decl();
            return address;
        };
        for (auto& field : node.pointerFields) {
            field.preValue = getPointerValue(field.preValue);
            field.postValue = getPointerValue(field.postValue);
        }
    };
    
    inline std::size_t GetExpansionDepth(const FlowGraphNode& node, std::size_t remainingDepth) {
        auto handle = [this,&remainingDepth,&node](auto& field){
            auto depth = graph.config.GetMaxFootprintDepth(node.address.type, field.name);
            remainingDepth = std::max(remainingDepth, depth);
        };
        for (const auto& field : node.dataFields) handle(field);
        for (const auto& field : node.pointerFields) handle(field);
        return remainingDepth;
    }
    
    std::set<const SymbolDeclaration*> ExpandGraph(std::size_t initialDepth) {
        Worklist worklist(initialDepth, graph.nodes.front());
        std::set<const SymbolDeclaration*> missingFrontier;

        // expand until maxDepth is reached
        while (!worklist.Empty()) {
            auto [depth, node] = worklist.Take();
            depth = GetExpansionDepth(*node, depth);
            InlineAliases(*node);
            if (depth == 0) continue;

            bool publish = !node->postLocal;
            for (auto& field : node->pointerFields) {
                for (auto mode : AllEMode) {
                    auto& nextAddress = field.Value(mode);
                    if (auto nextNode = TryGetOrCreateNode(nextAddress)) {
                        if (publish) nextNode->postLocal = false;
                        worklist.Add(depth - 1, *nextNode);
                    } else {
                        missingFrontier.insert(&nextAddress);
                    }
                }
            }
        }
        return missingFrontier;
    }
    
    void CheckGraph() const {
        // cyclic flow graphs are not supported // TODO: support cycles
        auto& root = graph.GetRoot();
        if (!graph.GetIncomingEdges(root, EMode::PRE).empty() ||
            !graph.GetIncomingEdges(root, EMode::POST).empty()) {
            throw std::logic_error("Unsupported update: cyclic flow graphs are not supported");
        }
    }
    
    inline FlowGraphNode& MakeRoot(const VariableDeclaration& root) {
        auto& rootAddress = plankton::GetResource(root, state).Value();
        FlowGraphNode* rootNode = TryGetOrCreateNode(rootAddress);
        if (!rootNode) throw std::logic_error("Cannot initialize flow graph."); // TODO: better error handling
        rootNode->needed = true;
        rootNode->preGraphInflow = rootNode->preAllInflow;
        rootNode->postGraphInflow = rootNode->preAllInflow;
        rootNode->postAllInflow = rootNode->preAllInflow;
        return *rootNode;
    }
    
    inline void DeriveFrontierKnowledge(const std::set<const SymbolDeclaration*>& frontier) {
        // get new memory
        auto& flowType = graph.config.GetFlowValueType();
        for (auto elem : frontier) DEBUG("  missing " << elem->name << ": nonnull=" << encoding.Implies(encoding.EncodeIsNonNull(*elem)) << std::endl)
        plankton::MakeMemoryAccessible(state, frontier, flowType, factory, encoding);
        encoding.AddPremise(encoding.EncodeInvariants(state, graph.config)); // for newly added memory
        encoding.AddPremise(encoding.EncodeSimpleFlowRules(state, graph.config)); // for newly added memory
        encoding.AddPremise(encoding.EncodeAcyclicity(state)); // for newly added memory
        
        // nodes with same address => same fields
        // prune duplicate memory
        plankton::ExtendStack(*graph.pre, encoding, ExtensionPolicy::POINTERS);
        assert(&state == graph.pre->now.get());
        plankton::InlineAndSimplify(*graph.pre);
    }
    
    void Construct(const VariableDeclaration& root, std::size_t depth) {
        plankton::ExtendStack(*graph.pre, encoding, ExtensionPolicy::POINTERS);
        plankton::InlineAndSimplify(*graph.pre);
        
        std::set<const SymbolDeclaration*> frontier;
        while (true) {
            MakeUpdates();

            encoding.Push();
            encoding.AddPremise(state);
            encoding.AddPremise(encoding.EncodeInvariants(state, graph.config));
            encoding.AddPremise(encoding.EncodeSimpleFlowRules(state, graph.config));
            encoding.AddPremise(encoding.EncodeAcyclicity(state));
            encoding.AddPremise(encoding.EncodeOwnership(state));
    
            MakeRoot(root);
            auto newFrontier = ExpandGraph(depth);
            if (frontier == newFrontier) break;
            frontier = std::move(newFrontier);
            
            DeriveFrontierKnowledge(frontier);
            encoding.Pop();
            graph.nodes.clear();
        }
        CheckGraph();
    }
};

inline void PostProcessFootprint(FlowGraph& footprint) {
    if constexpr (!POSTPROCESS_FLOW_GRAPHS) return;
    MEASURE("plankton::MakeFlowFootprint ~> PostProcessFootprint")

    Encoding encoding(footprint);
    auto handle = [&encoding](auto& pre, auto& post, const auto& info) {
        if (pre.get() == post.get()) return;
        auto equal = encoding.Encode(pre) == encoding.Encode(post);
        encoding.AddCheck(equal, [&pre,&post,info](bool holds){
            if (holds) post = pre.get();
        });
    };
    
    for (auto& node : footprint.nodes) {
        handle(node.preAllInflow, node.postAllInflow, "allInflow@" + node.address.name);
        handle(node.preGraphInflow, node.postGraphInflow, "graphInflow@" + node.address.name);
        handle(node.preKeyset, node.postKeyset, "keyset@" + node.address.name);
        for (auto& field : node.pointerFields) {
            handle(field.preAllOutflow, field.postAllOutflow, "allOutflow@" + node.address.name + "::" + field.name);
            handle(field.preGraphOutflow, field.postGraphOutflow, "graphOutflow@" + node.address.name + "::" + field.name);
        }
    }
    
    encoding.Check();
}

FlowGraph plankton::MakeFlowFootprint(std::unique_ptr<Annotation> pre, const MemoryWrite& command, const SolverConfig& config) {
    MEASURE("plankton::MakeFlowFootprint")

    assert(!command.lhs.empty());
    auto& lhs = *command.lhs.front();
    auto& root = lhs.variable->Decl();
    auto depth = config.GetMaxFootprintDepth(lhs.variable->Type(), lhs.fieldName);
    
    FlowGraph graph(std::move(pre), config);
    FlowGraphGenerator generator(graph, command);
    generator.Construct(root, depth);
    
    // ensure that all updates are covered by the footprint
    // Note: this guarantees that FlowGraphGenerator.updates are non-overlapping since graph.nodes are non-overlapping
    for (const auto& dereference : command.lhs) {
        auto target = graph.GetNodeOrNull(plankton::Evaluate(*dereference->variable, *graph.pre->now));
        if (target) continue;
        throw std::logic_error("Footprint construction failed: update to '" + plankton::ToString(*dereference) + "' not covered."); // TODO: better error handling
    }
    
    DEBUG("Footprint: " << std::endl)
    for (const auto& node : graph.nodes) {
        DEBUG("   - Node " << node.address.name << std::endl)
        for (const auto& next : node.pointerFields) {
            DEBUG("      - " << node.address.name << "->" << next.name << " == " << next.preValue.get().name << " / "
                             << next.postValue.get().name << std::endl)
        }
    }
    
    PostProcessFootprint(graph);
    return graph;
}

FlowGraph plankton::MakePureHeapGraph(std::unique_ptr<Annotation> state, SymbolFactory& factory, const SolverConfig& config) {
    MEASURE("plankton::MakePureHeapGraph")

    plankton::InlineAndSimplify(*state);
    FlowGraph graph(std::move(state), config);
    factory.Avoid(*graph.pre);

    // add all nodes
    auto memories = plankton::Collect<MemoryAxiom>(*graph.pre->now);
    for (const auto* memory : memories) {
        graph.nodes.push_back(MakeNodeFromResource(*memory, factory, graph));
    }

    // make pre/post state equal
    for (auto& node : graph.nodes) {
        node.needed = true;
        node.postAllInflow = node.preAllInflow;
        node.postKeyset = node.preKeyset;
        node.postGraphInflow = node.preGraphInflow;
        for (auto& field : node.pointerFields) {
            field.postAllOutflow = field.preAllOutflow;
            field.postGraphOutflow = field.preGraphOutflow;
        }
    }
    return graph;
}
