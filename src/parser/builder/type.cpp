#include "parser/builder.hpp"

#include "PlanktonBaseVisitor.h"

using namespace plankton;


std::string AstBuilder::MakeBaseTypeName(PlanktonParser::TypeContext& context) {
    struct : public PlanktonBaseVisitor {
        antlrcpp::Any visitTypeBool(PlanktonParser::TypeBoolContext*) override { return Type::Bool().name; }
        antlrcpp::Any visitTypeInt(PlanktonParser::TypeIntContext*) override { return Type::Data().name; }
        antlrcpp::Any visitTypeData(PlanktonParser::TypeDataContext*) override { return Type::Data().name; }
        antlrcpp::Any visitTypePtr(PlanktonParser::TypePtrContext* ctx) override { return ctx->name->getText(); }
    } visitor;
    return context.accept(&visitor);
}

const Type& AstBuilder::GetDummyNullType() {
    static Type dummy("__AstBuilder::DummyPtrType__", Sort::PTR);
    return dummy;
}