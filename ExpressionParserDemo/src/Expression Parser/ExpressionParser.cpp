#include "ExpressionParser.h"

#include <string>
#include <vector>
#include <cassert>


std::vector<std::string_view> SplitString(std::string_view input, std::string_view delimiter)
{
   std::vector<std::string_view> strings;
   size_t prevPosOfDelimiter = 0;
   size_t posOfDelimiter = input.find_first_of(delimiter);
   while (posOfDelimiter != std::string::npos)
   {
      strings.push_back(input.substr(prevPosOfDelimiter, posOfDelimiter - prevPosOfDelimiter));
      prevPosOfDelimiter = posOfDelimiter + delimiter.length();
      posOfDelimiter = input.find(delimiter, prevPosOfDelimiter);
   }

   if (prevPosOfDelimiter < input.length())
   {
      strings.push_back(input.substr(prevPosOfDelimiter));
   }

   return strings;
}


/****************************************
   Expression Parser
****************************************/

ExpressionParser::ParseResult ExpressionParser::Parse(std::string_view conditionalDataString)
{
   if (m_isValid == true)
   {
      // We have already got a valid tree, don't try to replace it
      return SetResult(ParseResult::AlreadyConstructed, 0);
   }

   if (conditionalDataString.empty())
   {
      return SetResult(ParseResult::EmptyStatement, 0);
   }

   std::vector<std::string_view> splitStrings = SplitString(conditionalDataString, " ");

   size_t currentLocationInString = 0;
   bool parsingCondition = false;

   for (std::string_view data : splitStrings)
   {
      parsingCondition = !parsingCondition;

      if (parsingCondition == true)
      {
         const size_t conditionStart = data.find_first_not_of('(');
         const size_t conditionEnd = data.find_last_not_of(')');
         std::string_view conditionData = data.substr(conditionStart, conditionEnd - conditionStart + 1);

         for (int charIndex = 0; charIndex < (int)conditionStart; ++charIndex)
         {
            if (data[charIndex] != '(')
            {
               // error
               Clear();
               return SetResult(ParseResult::ParsingInvalidCharacter, currentLocationInString);
            }

            OpenBrace();
            currentLocationInString++;
         }

         if (AddExpression(conditionData) == false)
         {
            Clear();
            return SetResult(ParseResult::InvalidExpression, currentLocationInString);
         }
         currentLocationInString += conditionData.length();

         for (size_t charIndex = conditionEnd + 1; charIndex < data.length(); ++charIndex)
         {
            if (data[charIndex] != ')')
            {
               Clear();
               return SetResult(ParseResult::ParsingInvalidCharacter, currentLocationInString);
            }

            if (CloseBrace() == false)
            {
               Clear();
               return SetResult(ParseResult::ClosingUnopenedBrace, currentLocationInString);
            }

            currentLocationInString++;
         }

         currentLocationInString++;    // Account for the space delimeter
         continue;
      }

      bool newLogicIsOr = data == "or" || data == "||";
      if (newLogicIsOr == false && data != "and" && data != "&&")
      {
         Clear();
         return SetResult(ParseResult::InvalidLogic, currentLocationInString);
      }

      currentLocationInString += data.length() + 1;    // +1 to account for the space delimeter

      if (m_pActiveBranchRoot->IsOrLogic() != newLogicIsOr)
      {
         // The logic is different, we need to swap the active roots
         m_pActiveBranchRoot = m_pActiveBranch->GetLogicPathRoot(!m_pActiveBranchRoot->IsOrLogic());

         if (newLogicIsOr == false)
         {
            // When switching from OR -> AND, we need to move the latest expression added to the OR root
            std::shared_ptr<Node> pNodeToMove = m_pActiveBranch->GetLogicPathRoot(true)->GetNext();
            m_pActiveBranchRoot->SetNext(pNodeToMove);
         }
      }
   }

   return SetResult(ParseResult::OK, 0);
}

bool ExpressionParser::AddExpression(std::string_view expressionString)
{
   std::shared_ptr<ExpressionNode> pExpressionNode = std::make_shared<ExpressionNode>(expressionString);
   if (pExpressionNode->IsValid() == false)
   {
      return false;
   }

   // We should always push expressions to the start of the branch, that way there if they satisfy
   // the requirements of the condition early we avoid doing unneeded calculations in higher branches
   m_pActiveBranchRoot->SetNext(pExpressionNode);
   return true;
}

void ExpressionParser::OpenBrace()
{
   std::shared_ptr<ForkNode> pNewFork = std::make_shared<ForkNode>();
   std::shared_ptr<BranchNode> pNewBranch = BranchNode::Create();
   pNewFork->SetLinkedRoot(pNewBranch->GetLogicPathRoot(true));
   m_braceForks.push(pNewFork);

   // Fork nodes are always pushed to the end to avoid unneeded extra calculations.
   m_pActiveBranchRoot->GetLast()->SetNext(pNewFork);
   m_pActiveBranchRoot = pNewFork->GetLinkedRoot();
}

bool ExpressionParser::CloseBrace()
{
   if (m_braceForks.empty())
   {
      return false;
   }

   std::shared_ptr<ForkNode> pResultNode = m_braceForks.top();
   m_pActiveBranchRoot = pResultNode->GetRoot();
   m_pActiveBranch = m_pActiveBranchRoot->GetParentBranch();
   m_braceForks.pop();
   return true;
}

void ExpressionParser::Clear()
{
   SetResult(ParseResult::OK, 0);
   m_isValid = false;
   m_pBaseBranch = BranchNode::Create();
   m_pActiveBranch = m_pBaseBranch;
   m_pActiveBranchRoot = m_pActiveBranch->GetLogicPathRoot(true);
   m_braceForks = std::stack<std::shared_ptr<ForkNode>>();
}

ExpressionParser::ParseResult ExpressionParser::SetResult(ParseResult result, size_t at)
{
   m_result = result;
   m_errorAt = at;
   return result;
}

std::string ExpressionParser::GetErrorMessage() const
{
   const std::string location = std::to_string(GetErrorLocation());
   switch (GetResultCode())
   {
      case ExpressionParser::ParseResult::EmptyStatement:            return "Cannot parse an empty statement string.";
      case ExpressionParser::ParseResult::AlreadyConstructed:        return "Logic has already been parsed successfully. Call 'Clear' before trying again.";
      case ExpressionParser::ParseResult::ClosingUnopenedBrace:      return "Found a closing brace without an open brace at " + location + ".";
      case ExpressionParser::ParseResult::InvalidExpression:         return "An invalid expression was found at " + location + ".";
      case ExpressionParser::ParseResult::InvalidLogic:              return "Invalid logic found at " + location + ". Only supports and/&& + or/||.";
      case ExpressionParser::ParseResult::ParsingInvalidCharacter:   return "An invalid character was found at " + location + ".";
      default:                                                          return "";
   }
}


/****************************************
   Nodes
****************************************/

void ExpressionParser::Node::SetNext(std::shared_ptr<Node> pMyNewNextNode)
{
   std::shared_ptr<Node> pMyOldNextNode = m_pNext;
   std::shared_ptr<Node> pTheirOldNextNode = pMyNewNextNode != nullptr ? pMyNewNextNode->m_pNext : nullptr;
   std::shared_ptr<Node> pTheirOldPrevNode = pMyNewNextNode != nullptr ? pMyNewNextNode->m_pPrev : nullptr;

   // Step 1 - Link the current and new nodes together
   m_pNext = pMyNewNextNode;
   if (pMyNewNextNode != nullptr)
   {
      pMyNewNextNode->m_pPrev = shared_from_this();
   }

   // Step 2 - Link the gap made by moving the new next node
   if (pTheirOldPrevNode != nullptr)
   {
      pTheirOldPrevNode->m_pNext = pMyNewNextNode != nullptr ? pMyNewNextNode->m_pNext : nullptr;
      if (pTheirOldPrevNode->m_pNext != nullptr)
      {
         pTheirOldPrevNode->m_pNext->m_pPrev = pTheirOldPrevNode;
      }
   }

   // Step 3 - Link the new next node to my old one
   if (pMyNewNextNode != nullptr)
   {
      pMyNewNextNode->m_pNext = pMyOldNextNode;
   }
   if (pMyOldNextNode != nullptr)
   {
      pMyOldNextNode->m_pPrev = pMyNewNextNode;
   }

   /*
   Current : 0 <--> 1 <--> 2 <--> 3 <--> 4
   I am Node 1
   Add new node 5 as my next
   Desired : 0 <--> 1 <--> 5 <--> 2 <--> 3 <--> 4

   my old next         = [2]    (old next 1)
   new node's old next = [NULL] (old next 2)
   new node's old prev = [NULL] (old prev 2)

   my (1) next is set to the new node
   1 ---> 5

   0 ---> 1 ---> 5      2 ---> 3 ---> 4

   0 <--- 1 <---------- 2 <--- 3 <--- 4


   the new nodes (5) prev is set to me (1)
   1 <--- 5

   0 ---> 1 ---> 5      2 ---> 3 ---> 4

   0 <--- 1 <--- 5
          1 <---------- 2 <--- 3 <--- 4


   the new nodes (5) old prev (NULL) next is set to the new nodes (5) next
   NULL - Not performed

   the new nodes (5) old next (NULL) prev is set to the new nodes (5) old prev (NULL)
   NULL - Not performed

   the new nodes (5) next is set to the my old next
   5 ---> 2

   0 ---> 1 ---> 5 ---> 2 ---> 3 ---> 4

   0 <--- 1 <--- 5
          1 <---------- 2 <--- 3 <--- 4


   my old next (2) prev is set to the new node
   5 <--- 2

   0 ---> 1 ---> 5 ---> 2 ---> 3 ---> 4
   0 <--- 1 <--- 5 <--- 2 <--- 3 <--- 4
   */

   /*
   Current : 0 <--> 1 <--> 2 <--> 3 <--> 4
   I am Node 1
   Set 4 as my next
   Desired : 0 <--> 1 <--> 4 <--> 2 <--> 3

   my old next         = [2]    (old next 1)
   new node's old next = [NULL] (old next 2)
   new node's old prev = [3]    (old prev 2)

   my (1) next is set to the new node
   1 ---> 4

   0 ---> 1 -----------------> 4
                 2 ---> 3 ---> 4

   0 <--- 1 <--- 2 <--- 3 <--- 4


   the new nodes (4) prev is set to me (1)
   1 <--- 4

   0 ---> 1 -----------------> 4
                 2 ---> 3 ---> 4

   0 <--- 1 <----------------- 4
          1 <--- 2 <--- 3


   the new nodes (4) old prev (3) next is set to the new nodes (4) next
   3 ---> 5

   0 ---> 1 -----------------> 4
                 2 ---> 3

   0 <--- 1 <----------------- 4
          1 <--- 2 <--- 3


   the new nodes (4) old next (NULL) prev is set to the new nodes (4) old prev (3)
   NULL - Not performed

   the new nodes (4) next is set to the my old next
   4 ---> 2

   0 ---> 1 ---> 4 ---> 2 ---> 3

   0 <--- 1 <----------------- 4
          1 <--- 2 <--- 3


   my old next (2) prev is set to the new node
   4 <--- 2

   0 ---> 1 ---> 4 ---> 2 ---> 3
   0 <--- 1 <--- 4 <--- 2 <--- 3
   */

   /*
   Current : 0 <--> 1 <--> 2 <--> 3 <--> 4 <--> 5
   I am Node 1
   Set 4 as my next
   Desired : 0 <--> 1 <--> 4 <--> 2 <--> 3 <--> 5

   my old next         = [2] (old next 1)
   new node's old next = [5] (old next 2)
   new node's old prev = [3] (old prev 2)

   my (1) next is set to the new node
   1 ---> 4

   0 ---> 1 -----------------> 4 ---> 5
                 2 ---> 3 ---> 4

   0 <--- 1 <--- 2 <--- 3 <--- 4 <--- 5


   the new nodes (4) prev is set to me (1)
   1 <--- 4

   0 ---> 1 -----------------> 4 ---> 5
                 2 ---> 3 ---> 4

   0 <--- 1 <----------------- 4 <--- 5
          1 <--- 2 <--- 3


   the new nodes (4) old prev (3) next is set to the new nodes (4) next
   3 ---> 5

   0 ---> 1 -----------------> 4 ---> 5
                 2 ---> 3 ----------> 5

   0 <--- 1 <----------------- 4 <--- 5
          1 <--- 2 <--- 3


   the new nodes (4) old next (5) prev is set to the new nodes (4) old prev (3)
   3 <--- 5

   0 ---> 1 -----------------> 4 ---> 5
                 2 ---> 3 ----------> 5

   0 <--- 1 <----------------- 4
          1 <--- 2 <--- 3 <---------- 5


   the new nodes (4) next is set to the my old next
   4 ---> 2

   0 ---> 1 ---> 4 ---> 2 ---> 3 ---> 5

   0 <--- 1 <----------------- 4
          1 <--- 2 <--- 3 <---------- 5


   my old next (2) prev is set to the new node
   4 <--- 2

   0 ---> 1 ---> 4 ---> 2 ---> 3 ---> 5
   0 <--- 1 <--- 4 <--- 2 <--- 3 <--- 5
   */
}

std::shared_ptr<ExpressionParser::BranchRootNode> ExpressionParser::Node::GetRoot()
{
   std::shared_ptr<Node> pNode = m_pPrev;
   if (pNode == nullptr)
   {
      return std::static_pointer_cast<BranchRootNode>(shared_from_this());
   }

   while (pNode->m_pPrev != nullptr)
   {
      pNode = pNode->m_pPrev;
   }
   return std::static_pointer_cast<BranchRootNode>(pNode);
}

std::shared_ptr<const ExpressionParser::BranchRootNode> ExpressionParser::Node::GetRoot() const
{
   std::shared_ptr<Node> pNode = m_pPrev;
   if (pNode == nullptr)
   {
      return std::static_pointer_cast<const BranchRootNode>(shared_from_this());
   }

   while (pNode->m_pPrev != nullptr)
   {
      pNode = pNode->m_pPrev;
   }
   return std::static_pointer_cast<const BranchRootNode>(pNode);
}

std::shared_ptr<ExpressionParser::Node> ExpressionParser::Node::GetLast()
{
   std::shared_ptr<Node> pNode = m_pNext;
   if (pNode == nullptr)
   {
      return std::static_pointer_cast<Node>(shared_from_this());
   }

   while (pNode->m_pNext != nullptr)
   {
      pNode = pNode->m_pNext;
   }
   return pNode;
}

std::shared_ptr<const ExpressionParser::Node> ExpressionParser::Node::GetLast() const
{
   std::shared_ptr<Node> pNode = m_pNext;
   if (pNode == nullptr)
   {
      return std::static_pointer_cast<const Node>(shared_from_this());
   }

   while (pNode->m_pNext != nullptr)
   {
      pNode = pNode->m_pNext;
   }
   return pNode;
}


/****************************************
   Branch Node
****************************************/

std::shared_ptr<ExpressionParser::BranchNode> ExpressionParser::BranchNode::Create()
{
   std::shared_ptr<BranchNode> pNewBranch = std::make_shared<BranchNode>(ConstructorKey{ 0 });
   pNewBranch->m_pOrRoot = std::make_shared<BranchRootNode>(pNewBranch, true);
   pNewBranch->m_pAndRoot = std::make_shared<BranchRootNode>(pNewBranch, false);

   std::shared_ptr<ForkNode> pFork = std::make_shared<ForkNode>();
   pFork->SetLinkedRoot(pNewBranch->m_pAndRoot);
   pNewBranch->m_pOrRoot->SetNext(pFork);

   return pNewBranch;
}

bool ExpressionParser::BranchNode::Evaluate(int value) const
{
   return m_pOrRoot->Evaluate(value);
}

/****************************************
   Expression Node
****************************************/

ExpressionParser::ExpressionNode::ExpressionNode(std::string_view data)
   : m_value(0)
   , m_operation(0)
{
   m_isValid = false;

   const size_t numberAt = data.find_first_of("0123456789");
   // "x0" or "xx0" where x is an operator of 1 or 2 characters wide
   if (numberAt != 1 && numberAt != 2)
   {
      return;
   }

   std::string_view comparisonOperator = data.substr(0, numberAt);
   if (comparisonOperator == "<") { m_operation = (int)LessThan; }
   else if (comparisonOperator == "<=") { m_operation = (int)LessThan | (int)EqualTo; }
   else if (comparisonOperator == "=") { m_operation = (int)EqualTo; }
   else if (comparisonOperator == "!=") { m_operation = (int)NotEqualTo; }
   else if (comparisonOperator == ">=") { m_operation = (int)GreaterThan | (int)EqualTo; }
   else if (comparisonOperator == ">") { m_operation = (int)GreaterThan; }
   else
   {
      return;
   }

   try
   {
      std::string_view valueString = data.substr(numberAt);
      m_value = std::stoi(valueString.data());
   }
   catch (...)
   {
      return;
   }

   m_isValid = true;
}

bool ExpressionParser::ExpressionNode::Evaluate(int value) const
{
   if (IsValid() == false)
   {
      return false;
   }

   bool result;
   switch (m_operation)
   {
      case (int)LessThan:                       result = value < m_value; break;
      case (int)LessThan | (int)EqualTo:        result = value <= m_value; break;
      case (int)EqualTo:                        result = value == m_value; break;
      case (int)NotEqualTo:                     result = value != m_value; break;
      case (int)GreaterThan | (int)EqualTo:     result = value >= m_value; break;
      case (int)GreaterThan:                    result = value > m_value; break;
      default: return false;
   }

   // The neat trick here is if result == true and IsOrLogic() == true, we return true as OR requires result == true at any point
   // On the other hand, if result == false and IsOrLogic() == false, we return false as AND requires result == true at ALL times
   // We also return the result if there is no next node to check
   if (GetNext() == nullptr || result == GetRoot()->IsOrLogic())
   {
      return result;
   }

   return GetNext()->Evaluate(value);
}


/****************************************
   Root Node
****************************************/

ExpressionParser::BranchRootNode::BranchRootNode(std::weak_ptr<BranchNode> pParent, bool isOrLogic)
   : m_pParentBranch(pParent)
   , m_isOrLogic(isOrLogic)
{}

bool ExpressionParser::BranchRootNode::Evaluate(int value) const
{
   if (GetNext() == nullptr)
   {
      // There is nothing on this branch
      return false;
   }
   return GetNext()->Evaluate(value);
}


/****************************************
   Fork Node
****************************************/

bool ExpressionParser::ForkNode::Evaluate(int value) const
{
   if (m_pBranchRoot == nullptr)
   {
      // This should never happen...
      assert(false);
      return false;
   }

   bool result = m_pBranchRoot->Evaluate(value);

   // The neat trick here is if result == true and IsOrLogic() == true, we return true as OR requires result == true at any point
   // On the other hand, if result == false and IsOrLogic() == false, we return false as AND requires result == true at ALL times
   // We also return the result if there is no next node to check
   if (GetNext() == nullptr || result == GetRoot()->IsOrLogic())
   {
      return result;
   }

   return GetNext()->Evaluate(value);
}