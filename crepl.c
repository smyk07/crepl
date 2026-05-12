#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Small dynamic array implementation
 */
typedef struct dynamic_array {
  void *items;
  unsigned int item_size;
  unsigned int count;
  unsigned int capacity;
} dynamic_array;

void dynamic_array_init(dynamic_array *da, unsigned int item_size) {
  da->items = NULL;
  da->item_size = item_size;
  da->count = 0;
  da->capacity = 0;
}

int dynamic_array_get(dynamic_array *da, unsigned int index, void *item) {
  if (!da || !item || index >= da->count || !da->items) {
    return -1;
  }

  void *source = (uint8_t *)da->items + (index * da->item_size);
  memcpy(item, source, da->item_size);
  return 0;
}

int dynamic_array_append(dynamic_array *da, const void *item) {
  if (!da || !item || da->item_size == 0) {
    perror("Invalid dynamic_array passed to append");
    return 1;
  }

  if (da->count >= da->capacity) {
    unsigned int new_capacity = da->capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 4;
    }

    void *new_items = realloc(da->items, new_capacity * da->item_size);
    if (new_items == NULL) {
      perror("Failed to reallocate dynamic array");
      return 1;
    }

    da->items = new_items;
    da->capacity = new_capacity;
  }

  memcpy((char *)da->items + (da->count * da->item_size), item, da->item_size);
  da->count++;

  return 0;
}

void dynamic_array_free(dynamic_array *da) {
  free(da->items);
  da->items = NULL;
  da->item_size = 0;
  da->count = 0;
  da->capacity = 0;
}

/*
 * Tokens
 */
typedef enum token_kind {
  TOKEN_INVALID = 0,

  TOKEN_TERM,

  TOKEN_OPERATION_ADDITION,
  TOKEN_OPERATION_SUBTRACTION,
  TOKEN_OPERATION_MULTIPLICATION,
  TOKEN_OPERATION_DIVISION,
  TOKEN_OPERATION_MODULO,
  TOKEN_OPERATION_EXPONENTIATION,

  TOKEN_BRACKET_OPEN,
  TOKEN_BRACKET_CLOSE,

  TOKEN_END,
} token_kind;

typedef struct token {
  token_kind kind;
  int value;
} token;

void tokenize(dynamic_array *tokens, char *buffer) {
  char *current = buffer;
  token token;

  do {
    if (*current == ' ') {
      current++;
      continue;
    } else if (*current == '\0') {
      token.kind = TOKEN_END;
      token.value = 0;
    } else if (isdigit(*current)) {
      token.kind = TOKEN_TERM;
      char *end;
      token.value = (int)strtol(current, &end, 10);
      current = end;
    } else if (*current == '+') {
      token.kind = TOKEN_OPERATION_ADDITION;
      token.value = 0;
      current++;
    } else if (*current == '-') {
      token.kind = TOKEN_OPERATION_SUBTRACTION;
      token.value = 0;
      current++;
    } else if (*current == '*') {
      token.kind = TOKEN_OPERATION_MULTIPLICATION;
      token.value = 0;
      current++;
    } else if (*current == '/') {
      token.kind = TOKEN_OPERATION_DIVISION;
      token.value = 0;
      current++;
    } else if (*current == '%') {
      token.kind = TOKEN_OPERATION_MODULO;
      token.value = 0;
      current++;
    } else if (*current == '^') {
      token.kind = TOKEN_OPERATION_EXPONENTIATION;
      token.value = 0;
      current++;
    } else if (*current == '(') {
      token.kind = TOKEN_BRACKET_OPEN;
      token.value = 0;
      current++;
    } else if (*current == ')') {
      token.kind = TOKEN_BRACKET_CLOSE;
      token.value = 0;
      current++;
    } else {
      token.kind = TOKEN_INVALID;
      token.value = 0;
      current++;
    }

    dynamic_array_append(tokens, &token);
  } while (token.kind != TOKEN_END);
}

/*
 * AST
 */
typedef enum expr_kind {
  EXPR_INVALID = 0,

  EXPR_TERM,

  EXPR_ADD,
  EXPR_SUBTRACT,
  EXPR_MULTIPLY,
  EXPR_DIVIDE,
  EXPR_MODULO,
  EXPR_EXPONENTIATION,

  EXPR_UNARY_NEGATE,
} expr_kind;

typedef struct expr_node {
  expr_kind kind;
  union {
    struct {
      struct expr_node *left;
      struct expr_node *right;
    } binary;

    struct {
      struct expr_node *operand;
    } unary;

    int term;
  };
} expr_node;

void free_expr(expr_node *expr) {
  if (!expr)
    return;

  switch (expr->kind) {
  case EXPR_UNARY_NEGATE:
    free_expr(expr->unary.operand);
    break;

  case EXPR_TERM:
    break;

  default:
    free_expr(expr->binary.left);
    free_expr(expr->binary.right);
    break;
  }

  free(expr);
}

/*
 * Parser
 */
typedef struct parser {
  dynamic_array *tokens;
  int position;
} parser;

void parser_init(dynamic_array *tokens, parser *p) {
  p->tokens = tokens;
  p->position = 0;
}

void parser_current(parser *p, token *token) {
  dynamic_array_get(p->tokens, p->position, token);
}

void parser_advance(parser *p) { p->position++; }

// Binding powers for each token
int lbp[] = {
    [TOKEN_INVALID] = 0,

    [TOKEN_TERM] = 0,

    [TOKEN_OPERATION_ADDITION] = 10,
    [TOKEN_OPERATION_SUBTRACTION] = 10,
    [TOKEN_OPERATION_MULTIPLICATION] = 20,
    [TOKEN_OPERATION_DIVISION] = 20,
    [TOKEN_OPERATION_MODULO] = 20,
    [TOKEN_OPERATION_EXPONENTIATION] = 30,

    [TOKEN_BRACKET_OPEN] = 0,
    [TOKEN_BRACKET_CLOSE] = 0,

    [TOKEN_END] = 0,
};

int rbp[] = {
    [TOKEN_INVALID] = 0,

    [TOKEN_TERM] = 0,

    [TOKEN_OPERATION_ADDITION] = 10,
    [TOKEN_OPERATION_SUBTRACTION] = 10,
    [TOKEN_OPERATION_MULTIPLICATION] = 20,
    [TOKEN_OPERATION_DIVISION] = 20,
    [TOKEN_OPERATION_MODULO] = 20,
    [TOKEN_OPERATION_EXPONENTIATION] = 29,

    [TOKEN_BRACKET_OPEN] = 0,
    [TOKEN_BRACKET_CLOSE] = 0,

    [TOKEN_END] = 0,
};

expr_node *parse_expr(parser *p, int bp) {
  token tok;
  parser_current(p, &tok);
  parser_advance(p);
  expr_node *left = NULL;

  // null denotation
  if (tok.kind == TOKEN_TERM) {
    left = malloc(sizeof(expr_node));
    left->kind = EXPR_TERM;
    left->term = tok.value;
  }

  else if (tok.kind == TOKEN_OPERATION_SUBTRACTION) {
    expr_node *operand = parse_expr(p, 40);
    if (!operand)
      return NULL;

    left = malloc(sizeof(expr_node));
    left->kind = EXPR_UNARY_NEGATE;
    left->unary.operand = operand;
  }

  else if (tok.kind == TOKEN_BRACKET_OPEN) {
    left = parse_expr(p, 0);
    if (!left)
      return NULL;

    parser_current(p, &tok);
    if (tok.kind != TOKEN_BRACKET_CLOSE) {
      fprintf(stderr, "Syntax error: expected ')'\n");
      free_expr(left);
      return NULL;
    }

    parser_advance(p);
  }

  else {
    fprintf(stderr, "Syntax error: unexpected token\n");
    return NULL;
  }

  // left denotation
  while (1) {
    parser_current(p, &tok);
    if (lbp[tok.kind] <= bp)
      break;

    parser_advance(p);

    expr_node *right = parse_expr(p, rbp[tok.kind]);
    if (!right) {
      free_expr(left);
      return NULL;
    }

    expr_node *node = malloc(sizeof(expr_node));

    switch (tok.kind) {
    case TOKEN_OPERATION_ADDITION:
      node->kind = EXPR_ADD;
      break;

    case TOKEN_OPERATION_SUBTRACTION:
      node->kind = EXPR_SUBTRACT;
      break;

    case TOKEN_OPERATION_MULTIPLICATION:
      node->kind = EXPR_MULTIPLY;
      break;

    case TOKEN_OPERATION_DIVISION:
      node->kind = EXPR_DIVIDE;
      break;

    case TOKEN_OPERATION_MODULO:
      node->kind = EXPR_MODULO;
      break;

    case TOKEN_OPERATION_EXPONENTIATION:
      node->kind = EXPR_EXPONENTIATION;
      break;

    default:
      fprintf(stderr, "Syntax error: unexpected operator\n");
      free_expr(left);
      free(node);
      return NULL;
    }

    node->binary.left = left;
    node->binary.right = right;
    left = node;
  }

  return left;
}

/*
 * Evaluation
 */
float evaluate_expr(expr_node *parent_expr) {
  switch (parent_expr->kind) {
  case EXPR_TERM:
    return (float)parent_expr->term;

  case EXPR_ADD:
    return evaluate_expr(parent_expr->binary.left) +
           evaluate_expr(parent_expr->binary.right);

  case EXPR_SUBTRACT:
    return evaluate_expr(parent_expr->binary.left) -
           evaluate_expr(parent_expr->binary.right);

  case EXPR_MULTIPLY:
    return evaluate_expr(parent_expr->binary.left) *
           evaluate_expr(parent_expr->binary.right);

  case EXPR_DIVIDE: {
    float divisor = evaluate_expr(parent_expr->binary.right);
    if (divisor == 0.0f) {
      fprintf(stderr, "Error: division by zero\n");
      return 0.0f;
    }
    return evaluate_expr(parent_expr->binary.left) / divisor;
  }

  case EXPR_MODULO: {
    float divisor = evaluate_expr(parent_expr->binary.right);
    if (divisor == 0.0f) {
      fprintf(stderr, "Error: modulo division by zero\n");
      return 0.0f;
    }
    return fmodf(evaluate_expr(parent_expr->binary.left), divisor);
  }

  case EXPR_EXPONENTIATION:
    return powf(evaluate_expr(parent_expr->binary.left),
                evaluate_expr(parent_expr->binary.right));

  case EXPR_UNARY_NEGATE:
    return -evaluate_expr(parent_expr->unary.operand);

  default:
    fprintf(stderr, "Error: unknown expr_kind %d\n", parent_expr->kind);
    return 0.0f;
  }
}

int main() {
  while (1) {
    char buffer[1024];

    printf("\n>>> ");
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
      printf("\n");
      break;
    }
    buffer[strcspn(buffer, "\n")] = '\0';

    dynamic_array tokens;
    dynamic_array_init(&tokens, sizeof(token));
    tokenize(&tokens, buffer);

    parser p;
    parser_init(&tokens, &p);
    expr_node *root_expr = parse_expr(&p, 0);

    if (root_expr)
      printf("  = %.2f\n", evaluate_expr(root_expr));

    free_expr(root_expr);
    dynamic_array_free(&tokens);
  }
}
