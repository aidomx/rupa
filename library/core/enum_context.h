#pragma once

enum ContextType {
  CONTEXT_VAR_DECLARATION,       // x,y
  CONTEXT_CONST_VAR_DECLARATION, // PI = 3.14
  CONTEXT_VAR_DESTRUCTURING,     // {x, y} = point

  CONTEXT_FUNCTION_DECLARATION, // fn() {}
  // CONTEXT_FUNCTION_EXPRESSION,  // f = fn() {}
  // CONTEXT_ARROW_FUNCTION,       // f = () => {}
  // CONTEXT_GENERATOR_FUNCTION,   // fn*() {}
  CONTEXT_WAIT_FUNCTION, // wait fn() {}

  // CONTEXT_CLASS_DECLARATION,        // class MyClass {}
  CONTEXT_STRUCT_DECLARATION, // Point {}
  CONTEXT_STRUCT_ASSIGNMENT,  // x = {}
  // CONTEXT_INTERFACE_DECLARATION,    // interface Serializable {}
  CONTEXT_CONST_STRUCT_DECLARATION, // const Color {}
  // CONTEXT_TYPE_DECLARATION,         // type Point = {x: number}

  CONTEXT_EXTENDS_DECLARATION, // extends type
  CONTEXT_IMPORT_DECLARATION,  // import x from module
  CONTEXT_EXPORT_DECLARATION,  // export foo() {}
  CONTEXT_MODULE_DECLARATION,  // module MyModule {}

  // ==================== EXECUTION CONTEXTS ====================
  CONTEXT_FUNCTION_CALL, // myFn()
  CONTEXT_METHOD_CALL,   // object.method()
  // CONTEXT_CONSTRUCTOR_CALL, // new MyClass()
  // CONTEXT_SUPER_CALL,       // super()

  CONTEXT_EXPRESSION,            // 1 + 2 * 3
  CONTEXT_ARITHMETIC_EXPRESSION, // 2 + 3 * 4
  CONTEXT_LOGICAL_EXPRESSION,    // a && b || c
  CONTEXT_COMPARISON_EXPRESSION, // x > y
  CONTEXT_ASSIGNMENT_EXPRESSION, // x = value
  CONTEXT_TERNARY_EXPRESSION,    // condition ? a : b

  CONTEXT_OBJECT_LITERAL,   // {key: value}
  CONTEXT_ARRAY_LITERAL,    // [1, 2, 3]
  CONTEXT_STRING_LITERAL,   // "hello"
  CONTEXT_TEMPLATE_LITERAL, // `hello ${name}`
  CONTEXT_NUMBER_LITERAL,   // 42, 3.14
  CONTEXT_REGEX_LITERAL,    // /pattern/flags

  // ==================== FLOW CONTROL CONTEXTS ====================
  CONTEXT_IF_STATEMENT,        // if condition {}
  CONTEXT_ELSEIF_STATEMENT,    // elseif condition {}
  CONTEXT_ELSE_STATEMENT,      // else {}
  CONTEXT_SWITCH_STATEMENT,    // switch (value) {}
  CONTEXT_CASE_CLAUSE,         // case value:
  CONTEXT_CONDITIONAL_TERNARY, // condition ? a : b

  CONTEXT_FOR_LOOP,      // for init, condition {}
  CONTEXT_FOR_IN_LOOP,   // for init in object {}
  CONTEXT_FOR_OF_LOOP,   // for init of array {}
  CONTEXT_REV_LOOP,      // rev {}
  CONTEXT_WHILE_LOOP,    // while condition {}
  CONTEXT_DO_WHILE_LOOP, // do {} while condition
  CONTEXT_LOOP_LABEL,    // label: for {}

  CONTEXT_PRINT_STATEMENT,

  CONTEXT_RETURN_STATEMENT,   // return value
  CONTEXT_BREAK_STATEMENT,    // break
  CONTEXT_CONTINUE_STATEMENT, // continue
  CONTEXT_THROW_STATEMENT,    // throw error
  CONTEXT_TRY_BLOCK,          // try {}
  CONTEXT_CATCH_BLOCK,        // catch (e) {}
  CONTEXT_FINALLY_BLOCK,      // finally {}
  CONTEXT_BLOCK_END,          // }

  // ==================== SCOPE & BLOCK CONTEXTS ====================
  CONTEXT_BLOCK_STATEMENT, // { statements }
  CONTEXT_FUNCTION_BODY,   // function body
  CONTEXT_CLASS_BODY,      // class body
  CONTEXT_MODULE_BODY,     // module body

  CONTEXT_GLOBAL_SCOPE,   // global scope
  CONTEXT_FUNCTION_SCOPE, // function scope
  CONTEXT_BLOCK_SCOPE,    // block scope (let/const)
  CONTEXT_MODULE_SCOPE,   // module scope
  CONTEXT_LEXICAL_SCOPE,  // lexical scope

  // ==================== SPECIAL CONTEXTS ====================
  CONTEXT_TYPE_ANNOTATION,   // : number
  CONTEXT_GENERIC_TYPE,      // Array<string>
  CONTEXT_UNION_TYPE,        // string | number
  CONTEXT_INTERSECTION_TYPE, // A & B
  CONTEXT_TYPE_ASSERTION,    // value as Type

  CONTEXT_DECORATOR,          // @decorator
  CONTEXT_ATTRIBUTE,          // [Attribute]
  CONTEXT_MACRO,              // #macro
  CONTEXT_COMPILER_DIRECTIVE, // #pragma, #ifdef

  // CONTEXT_AWAIT_EXPRESSION, // await promise
  // CONTEXT_PROMISE_CHAIN,    // .then().catch()
  // CONTEXT_ASYNC_ITERATION,  // for await ()

  CONTEXT_MEMORY_ALLOCATION,   // malloc, new
  CONTEXT_MEMORY_DEALLOCATION, // free, delete
  CONTEXT_GARBAGE_COLLECTION,  // GC-related

  // ==================== RUPA-SPECIFIC CONTEXTS ====================
  CONTEXT_IDENTIFIER_REFERENCE,  // x (standalone identifier)
  CONTEXT_IMPLICIT_FUNCTION_DEF, // fn() {} (tanpa keyword)
  CONTEXT_IMPLICIT_STRUCT_DEF,   // Point { x, y } (tanpa keyword)
  CONTEXT_REPL_INPUT,            // REPL-specific context
  CONTEXT_PARTIAL_STATEMENT,     // Incomplete input
  CONTEXT_CONTINUATION,          // Line continuation
  CONTEXT_UNKNOWN,               // Unknown context (fallback)

  CONTEXT_COUNT // Total number of contexts (for validation)
};
