// Copyright (c) 2017, eaugeas <eaugeas at gmail dot com>

#ifndef TEST_TEST_H_
#define TEST_TEST_H_

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct test_ctx_s {
  char *test_expr;
  int failure;
  int success;
  int verbose;
} test_ctx_t;

#define COND_EQ(a, b) ((a) == (b))
#define COND_NEQ(a, b) (!COND_EQ(a, b))
#define COND_MEM_EQ(a, b, l) (memcmp((a), (b), (l)) == 0)
#define COND_TRUE(a) COND_EQ(!!a, 1)
#define COND_FALSE(a) COND_EQ(!a, 1)

#define STRING_ERROR "assertion error %s %s at line %d: "

#define PRINT_ERR(a, b, COND)                            \
  fprintf(stderr, STRING_ERROR #a " " #COND " " #b "\n", \
          __FILE__, __FUNCTION__, __LINE__);

#define ASSERT_COND1(a, COND)   \
  if (!COND(a)) {               \
    PRINT_ERR(a, a, COND)       \
    return EXIT_FAILURE;        \
  }

#define ASSERT_COND2(a, b, COND) \
  if (!COND(a, b)) {             \
    PRINT_ERR(a, b, COND)        \
    return EXIT_FAILURE;         \
  }

#define ASSERT_COND3(a, b, n, COND) \
  if (!COND(a, b, n)) {             \
    PRINT_ERR(a, b, COND)           \
    return EXIT_FAILURE;            \
  }

#define ASSERT_MEM_EQ(a, b, l) ASSERT_COND3(a, b, l, COND_MEM_EQ)
#define ASSERT_NEQ(a, b) ASSERT_COND2(a, b, COND_NEQ)
#define ASSERT_EQ(a, b) ASSERT_COND2(a, b, COND_EQ)
#define ASSERT_TRUE(a) ASSERT_COND1(a, COND_TRUE)
#define ASSERT_FALSE(a) ASSERT_COND1(a, COND_FALSE)
#define ASSERT_NULL(a) ASSERT_COND2(a, NULL, COND_EQ)

static void
__test_print_help(const char *prog) {
  printf("usage: %s [-v] [-h] [-t]\n", prog);
  printf("\noptions:\n");
  printf("\t-v, --verbose: outputs information about tests run\n");
  printf("\t-h, --help: prints this menu\n");
  printf("\t-t, --test: expression to match against tests to run\n");
  printf("\n");
}

static void
__test_release(test_ctx_t *ctx) {
  free(ctx->test_expr);
  printf("test sucess: %d, failed: %d\n", ctx->success, ctx->failure);
}

static int
__test_ctx_init(test_ctx_t *ctx,
                int argc,
                char *argv[]) {
  ctx->test_expr = (char*) malloc(128 * sizeof(char));
  memset(ctx->test_expr, 0, 128);
  ctx->failure = 0;
  ctx->success = 0;
  ctx->verbose = 0;

  while (1) {
    static struct option long_options[] = {
      {"test", required_argument, 0, 't'},
      {"verbose", no_argument, 0, 'v'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };

    size_t len;
    int option_index = 0;
    int c = getopt_long(argc, argv, "t:vh", long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
    case 't':
      len = strlen(optarg) > 127 ? 127 : strlen(optarg);
      strncpy(ctx->test_expr, optarg, len);
      ctx->test_expr[len] = '\0';
      break;
    case 'v':
      ctx->verbose = 1;
      break;
    case 'h':
      __test_print_help(argv[0]);
      __test_release(ctx);
      return -11;
    case '?':
      // getopt_long already printed an error message
      return -1;
    default:
      break;
    }
  }

  return 0;
}

#define TEST_RELEASE(ctx) __test_release(&ctx);

#define TEST_INIT(ctx, argc, argv)         \
  if (__test_ctx_init(&ctx, argc, argv)) { \
    return EXIT_SUCCESS;                   \
  }                                        \

#define TEST_RUN(ctx, test)                                  \
  if (strlen(ctx.test_expr) == 0 ||                          \
      strstr(#test, ctx.test_expr) != NULL) {                \
    if (ctx.verbose) printf("running " #test "\n");          \
    int result = test();                                     \
    if (result == EXIT_FAILURE) {                            \
      if (ctx.verbose) printf("test " #test " failed\n");    \
      ctx.failure++;                                         \
    } else {                                                 \
      if (ctx.verbose) printf("test " #test " succeeded\n"); \
      ctx.success++;                                         \
    }                                                        \
  }                                                          \

#endif  // TEST_TEST_H_
