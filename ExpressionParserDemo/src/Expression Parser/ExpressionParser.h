#pragma once

#include <memory>
#include <string_view>
#include <stack>

class ExpressionParser
{
   struct BranchRootNode;
   struct Node : public std::enable_shared_from_this<Node>
   {
      Node()
         : m_pNext(nullptr)
         , m_pPrev(nullptr)
      {}

      virtual bool Evaluate(int value) const = 0;

      // Set the node following this one. Automatically reorganises the linkage between
      // nodes before and after should it be needed.
      void SetNext(std::shared_ptr<Node> pMyNewNextNode);

      std::shared_ptr<Node> GetNext() const { return m_pNext; }
      std::shared_ptr<Node> GetPrev() const { return m_pPrev; }

      std::shared_ptr<const BranchRootNode> GetRoot() const;
      std::shared_ptr<BranchRootNode> GetRoot();
      std::shared_ptr<const Node> GetLast() const;
      std::shared_ptr<Node> GetLast();

   private:
      std::shared_ptr<Node> m_pNext;
      std::shared_ptr<Node> m_pPrev;
   };

   // Branch nodes are a container for OR and AND logic paths which hold the actual expressions.
   struct BranchNode : public Node
   {
   private:
      // This private struct is only used to keep BranchNode constructor public, meaning we can
      // use std::make_shared only in the Create() function. Trying to create a BranchNode without
      // using Create() will fail.
      struct ConstructorKey { explicit ConstructorKey(int) {}; };

   public:
      BranchNode(const ConstructorKey&)
         : m_pOrRoot(nullptr)
         , m_pAndRoot(nullptr)
      {}

      virtual bool Evaluate(int value) const override;

      static std::shared_ptr<BranchNode> Create();

      // Get the OR or AND logic path root node.
      std::shared_ptr<BranchRootNode> GetLogicPathRoot(bool orLogic) { return orLogic ? m_pOrRoot : m_pAndRoot; }

   private:
      std::shared_ptr<BranchRootNode> m_pOrRoot;
      std::shared_ptr<BranchRootNode> m_pAndRoot;
   };

   // Root nodes are the first nodes along a branch, they hold the logic information
   // the following nodes will use when calling Evaluate().
   struct BranchRootNode : public Node
   {
      BranchRootNode(std::weak_ptr<BranchNode> pParent, bool isOrLogic);

      // Starts the evaluation of the current branch.
      virtual bool Evaluate(int value) const override;

      std::shared_ptr<BranchNode> GetParentBranch() const { return m_pParentBranch.lock(); }

      bool IsOrLogic() const { return m_isOrLogic; }

   private:
      std::weak_ptr<BranchNode> m_pParentBranch;
      bool m_isOrLogic;
   };

   // Fork nodes indicate where a branch has been created. When a forked branch has
   // been evaluated, the result will be returned here.
   struct ForkNode : public Node
   {
      ForkNode()
         : m_pBranchRoot(std::shared_ptr<BranchRootNode>(nullptr))
      {}

      virtual bool Evaluate(int value) const override;

      // Set/get the branch root this fork links to.
      void SetLinkedRoot(std::shared_ptr<BranchRootNode> pBranchRoot) { m_pBranchRoot = pBranchRoot; }
      std::shared_ptr<BranchRootNode> GetLinkedRoot() const { return m_pBranchRoot; }

   private:
      std::shared_ptr<BranchRootNode> m_pBranchRoot;
   };

   // Expression nodes are where the actual comparisons occur. These nodes convert
   // a string condition in the form <operator><value> (eg: "<5" or ">=100") and will
   // evaluate if the given value fits the requirements.
   struct ExpressionNode : public Node
   {
      enum OperatorFlags
      {
         LessThan = 1 << 0,
         GreaterThan = 1 << 1,
         EqualTo = 1 << 2,
         NotEqualTo = 1 << 3,
      };

      ExpressionNode(std::string_view expressionString);

      virtual bool Evaluate(int value) const override;

      // Check to see if the expression was created without errors.
      bool IsValid() const { return m_isValid; }

   private:
      bool m_isValid;
      int m_operation;
      int m_value;
   };

public:
   enum class ParseResult : uint8_t
   {
      OK = 0,

      AlreadyConstructed,
      EmptyStatement,
      ParsingInvalidCharacter,
      ClosingUnopenedBrace,
      InvalidExpression,
      InvalidLogic
   };

   ExpressionParser()
   {
      Clear();
   }

   bool Evaluate(const int& value) const { return m_pBaseBranch->Evaluate(value); }

   // Parse a logical expression. The string must be in the following format:
   // <expression> (<logic> <expression>)...
   // At least 1 expression is required, however you can also add logic (and/or) as well
   // to make a more complex condition. You can also add brackets to group certain
   // expressions together.
   // eg: "(>3 or <10) and !=5"
   // Returns a ParseResult code, with ParseResult::OK being a success and anything else
   // being a failure.
   ParseResult Parse(std::string_view conditionalDataString);

   void Clear();

   size_t GetErrorLocation() const { return m_errorAt; }
   ParseResult GetResultCode() const { return m_result; }
   std::string GetErrorMessage() const;

private:
   void OpenBrace();
   bool CloseBrace();

   bool AddExpression(std::string_view condition);

   ParseResult SetResult(ParseResult result, size_t at);

private:
   bool m_isValid;
   std::shared_ptr<BranchNode> m_pBaseBranch;
   std::shared_ptr<BranchNode> m_pActiveBranch;
   std::shared_ptr<BranchRootNode> m_pActiveBranchRoot;
   std::stack<std::shared_ptr<ForkNode>> m_braceForks;

   ParseResult m_result;
   size_t m_errorAt;
};