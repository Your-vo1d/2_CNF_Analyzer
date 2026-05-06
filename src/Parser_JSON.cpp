// Parser_JSON implementation has been split into logical modules:
//   Parser_JSON_Core.cpp      — constructor, destructor, parseJsonFile, printElements, printCNFAsVectors
//   Parser_JSON_Row.cpp       — processRowObject, processCallRow, substituteParams, tryProcessArrowExpression
//   Parser_JSON_Structure.cpp — processStructureField, navigateByField, processNestedValue, processBody, processLoop
//   Parser_JSON_Memory.cpp    — handleMemoryAllocation, handleFree, handleNullAssignment, handleSpecialPointerAssignment
//   Parser_JSON_Tree.cpp      — mergeTrees, findLeaves, findOrCreateElement, removeRootLinkClauses, findSetBitPosition
//   Parser_JSON_CNF.cpp       — solveCNF
