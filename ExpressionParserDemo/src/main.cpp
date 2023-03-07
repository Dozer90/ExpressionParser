#include "Expression Parser/ExpressionParser.h"

#include <iostream>
#include <string>

const std::string ReadFromStream()
{
   std::string valIn;
   std::getline(std::cin, valIn);
   return valIn;
};

int main()
{
   std::cout << "=======================================================================================================================" << std::endl;
   std::cout << "This is a demo to showcase how the ExpressionParser class can parse a string expression then evaluate an integer value." << std::endl;
   std::cout << std::endl;
   std::cout << "Expressions must be in the following format: \"COMP <LOGIC COMP>...\" where:" << std::endl;
   std::cout << " - 'COMP' is a comparison operation (<, <=, >, >=, =, !=) and a value with no space between them" << std::endl;
   std::cout << " - 'LOGIC' is either \"and\" or \"or\" (interchangable with the progammatic operators \"&&\" or \"||\")" << std::endl;
   std::cout << " - additional logic is optional, however if used a comparison must proceed it" << std::endl;
   std::cout << " - braces may be used to change the order of operations" << std::endl;
   std::cout << std::endl;
   std::cout << "Examples:" << std::endl;
   std::cout << " - \"<=100\"" << std::endl;
   std::cout << " - \">=0 && <=100 && !=50\"" << std::endl;
   std::cout << " - \">10 and <50 or >100\" ('and' only passes if >10 and <50)" << std::endl;
   std::cout << " - \">10 and (<50 or >100)\" ('and' passes if <50 OR >100 due to braces)" << std::endl;
   std::cout << "=======================================================================================================================" << std::endl;
   std::cout << std::endl;

   ExpressionParser parser;
   while (true)
   {
      parser.Clear();
      std::cout << "Type an expression (or 'end' to quit): ";

      std::string expressionIn = ReadFromStream();
      if (expressionIn == "end")
      {
         break;
      }

      if (parser.Parse(expressionIn) != ExpressionParser::ParseResult::OK)
      {
         std::cout << "Expression failed: " << parser.GetErrorMessage() << std::endl << std::endl;
         continue;
      }

      // Test different inputs on the generated logic tree
      while (true)
      {
         std::cout << std::endl << "Type an input to evaluate (or 'end' to stop testing): ";

         const std::string valueIn = ReadFromStream();
         if (valueIn == "end")
         {
            break;
         }

         int value;
         try
         {
            value = std::stoi(valueIn);
         }
         catch (...)
         {
            std::cout << "Invalid input.";
            continue;
         }
         std::cout << "Evaluated result: " << (parser.Evaluate(value) == true ? "TRUE" : "FALSE") << std::endl;
      }
   }
}