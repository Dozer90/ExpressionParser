# ExpressionParser
A custom build expression parser that can convert a string of conditions with and/or logic and braces that can then be evaluated with an integer value.

I was working on an system that is completely data driven with conditions being setup based on JSON files, and of those conditions a common requirement was comparing a value to certain conditions. This small class was constructed to parse conditional expressions with basic and/or logic (including braces) and then evaluate a value returning true or false if the condition is met.

Expressions must be in the following format: "COMP \<LOGIC COMP>..." where:
 - 'COMP' is a comparison operation (<, <=, >, >=, =, !=) and a value with no space between them
 - 'LOGIC' is either "and" or "or" (interchangable with the progammatic operators "&&" or "||")
 - logic is optional, however if used a comparison must proceed it
 - braces may be used to change the order of operations

Examples:
 - "<=100"
 - ">=0 && <=100 && !=50"
 - ">10 and <50 or >100" ('and' only passes if >10 and <50)
 - ">10 and (<50 or >100)" ('and' passes if <50 OR >100 due to braces)
