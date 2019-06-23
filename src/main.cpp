#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include <sstream>

using namespace clang;
using namespace clang::tooling;
using namespace clang::driver;
using namespace llvm;

using namespace std;

bool isSystemCModule(const CXXRecordDecl* decl) {
  string name = decl->getQualifiedNameAsString();
  //errs() << "name = " << name << "\n";
  if ((name == "sc_core::sc_module")) {
    return true;
  }

  if (decl->hasDefinition()) {
    if (decl->getNumBases() > 0) {

      for (auto base : decl->bases()) {
        const clang::Type* tp = base.getType().getTypePtr();
        auto recTp = tp->getAs<clang::RecordType>();
        if (recTp != nullptr) {
          auto baseDecl = recTp->getDecl();

          if (CXXRecordDecl::classof(baseDecl)) {
            if (isSystemCModule(dyn_cast<const clang::CXXRecordDecl>(baseDecl))) {
              return true;
            }
          }
        }
      }
    }
  }

  return false;
}

class FindNamedClassVisitor
  : public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
  explicit FindNamedClassVisitor(ASTContext *Context)
    : Context(Context) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *decl) {

    vector<RecordDecl*> inputSignals;
    
    if (isSystemCModule(decl)) {
      errs() << "Found SystemC module " << decl->getQualifiedNameAsString() << "\n";
      SourceManager& mgr = Context->getSourceManager();
      
      for (auto ctor : decl->ctors()) {
        errs() << "\tConstructor body\n";
        Stmt* body = ctor->getBody();
        if (body != nullptr) {
          if (CompoundStmt::classof(body)) {
            auto stmts = dyn_cast<CompoundStmt>(body);
            for (Stmt* st : stmts->children()) {
              errs() << "--- Body stmt\n";
              SourceLocation start = st->getBeginLoc();
              if (mgr.isMacroBodyExpansion(start)) {
                errs() << "\t\t### Is in macro\n";
                SourceLocation topMacro =
                  mgr.getTopMacroCallerLoc(start);
                errs() << "\t\tTop macro location = " << topMacro.printToString(mgr) << "\n";

                LangOptions LangOpts;
                StringRef subExprText =
                  Lexer::getSourceText(CharSourceRange::getTokenRange(st->getSourceRange()), mgr, LangOpts);
                errs() << "Macro subexpr = " << subExprText << "\n";

                //errs() << "\t\tFile location of macro = " << mgr.getFilename(topMacro) << "\n";
              }
              st->dump();
            }
          }
        }
      }

      errs() << "--- Fields\n";
      for (auto fd : decl->fields()) {
        QualType qtp = fd->getType();
        const clang::Type* tp = qtp.getTypePtr();
        tp->dump();
        errs() << "\t" << qtp.getAsString() << " " << fd->getName() << "\n";

        if (RecordType::classof(tp)) {
          errs() << "\t\tis a record\n";
          const RecordType* declTp = dyn_cast<const RecordType>(tp);
          const RecordDecl* d = declTp->getDecl();
          if (CXXRecordDecl::classof(d)) {
            const CXXRecordDecl* cxxDecl = dyn_cast<const CXXRecordDecl>(d);
            if (isSystemCModule(cxxDecl)) {
              errs() << "\t\t\tis a system c module\n";
            }
          }
        } else if (TemplateSpecializationType::classof(tp)) {
          errs() << "\t\tis template specialization\n";
          auto templateSpec =
            dyn_cast<const TemplateSpecializationType>(tp);
          TemplateName templateName =
            templateSpec->getTemplateName();

          string str;
          raw_string_ostream ss(str);
          templateName.dump(ss);
          errs() << "template name is " << ss.str() << "\n";
        } else if (TypedefType::classof(tp)) {
          errs() << "\t\tis a typedef\n";
        }
      }
    }
    
    return true;
  }

private:
  ASTContext* Context;
};

class FindNamedClassConsumer : public clang::ASTConsumer {
public:
  explicit FindNamedClassConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  FindNamedClassVisitor Visitor;
};

class FindNamedClassAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new FindNamedClassConsumer(&Compiler.getASTContext()));
  }
};

class FindNamedClassActionFactory : public FrontendActionFactory {
public:
  virtual FrontendAction* create() override {
    return new FindNamedClassAction();
  }
};

static cl::OptionCategory MyToolCategory("My tool options");

int main(int argc, const char **argv) {

  CommonOptionsParser options(argc, argv, MyToolCategory);
  const auto& sources = options.getSourcePathList();
  auto& db = options.getCompilations();
  
  ClangTool tool(db, sources);

  FindNamedClassActionFactory factory;
  tool.run(&factory);

  errs() << "Done\n";
  // if (argc > 1) {
  //   clang::tooling::runToolOnCode(new FindNamedClassAction, argv[1]);
  // }
}
