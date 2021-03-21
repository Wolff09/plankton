#pragma once
#ifndef HEAL_PROPERTIES
#define HEAL_PROPERTIES


#include <map>
#include <array>
#include <memory>
#include "cola/ast.hpp"
#include "logic.hpp"
#include "util.hpp"


namespace heal {

	struct PropertyInstantiationError : public std::exception {
		const std::string cause;
		PropertyInstantiationError(std::string name, std::size_t index, const cola::Type& expected, const cola::Type& got)
			: cause("Instantiation of property '" + std::move(name) + "' failed: expected argument at position " + std::to_string(index+1) +
		                " to be of type '" + expected.name + "' but got incompatible type '" + got.name + "'." ) {}
		[[nodiscard]] const char* what() const noexcept override { return cause.c_str(); }
	};


	template<std::size_t N = 1, typename T = SeparatingConjunction, typename = std::enable_if_t<std::is_base_of_v<Formula, T>>>
	struct Property {
		std::string name;
		std::array<std::unique_ptr<cola::VariableDeclaration>, N> vars;
		std::unique_ptr<T> blueprint;

		Property(std::string name_, std::array<std::unique_ptr<cola::VariableDeclaration>, N> vars_, std::unique_ptr<T> blueprint_)
			: name(std::move(name_)), vars(std::move(vars_)), blueprint(std::move(blueprint_))
		{
			assert(!name.empty());
			assert(blueprint);
			for (const auto& var : vars) assert(var);
		}

		std::unique_ptr<T> instantiate(std::array<std::reference_wrapper<const cola::VariableDeclaration>, N> decls) const {
			// create lookup map
			std::map<const cola::VariableDeclaration*, const cola::VariableDeclaration*> dummy2real;
			for (std::size_t index = 0; index < vars.size(); ++index) {
				const auto& var_type = vars.at(index)->type;
				const auto& decl_type = decls.at(index).get().type;
				if (!cola::assignable(var_type, decl_type)) {
					throw PropertyInstantiationError(name, index, var_type, decl_type);
				}
				dummy2real[vars.at(index).get()] = &decls.at(index).get();
			}

			// create instantiation
			return heal::Replace(heal::Copy(*blueprint), [&dummy2real](const auto& var) {
                auto find = dummy2real.find(&var.decl);
                if (find != dummy2real.end()) {
                    return std::make_pair(true, std::make_unique<LogicVariable>(*find->second));
                }
                return std::make_pair(false, nullptr);
            });
		}

		template<typename... Targs>
		std::unique_ptr<T> instantiate(const Targs&... args) const {
			std::array<std::reference_wrapper<const cola::VariableDeclaration>, N> decls { args... };
			return instantiate(std::move(decls));
		}
	};


	using Invariant = Property<1, SeparatingConjunction>;
	using Predicate = Property<2, SeparatingConjunction>;

} // namespace heal

#endif