#include <errno.h>

#include "../src/log.h"
#include "test.h"

int test_log_create()
{
	log_t* l = log_create();
	ASSERT_EQ(l->props, NULL);
	log_free(l);

	return 0;
}

static int init_test_log(log_t* log)
{
	log_init(log);

	ASSERT_EQ(log_get(log, "A"), NULL);

	log->props = malloc(sizeof(prop_t));
	log->props->key = "A";
	log->props->value = "a";
	log->props->next = NULL;

	return 0;
}

int test_log_get()
{
	log_t log;

	ASSERT_EQ(init_test_log(&log), 0);

	const char* expected = log_get(&log, "A");
	ASSERT_NEQ(expected, NULL);
	ASSERT_MEM_EQ(expected, "a", 2);

	expected = log_get(&log, "B");
	ASSERT_EQ(expected, NULL);

	free(log.props);
	return 0;
}

int test_log_set()
{
	log_t log;

	prop_t* prop = malloc(sizeof(prop_t));

	log_set(&log, prop, "A", "a");

	ASSERT_NEQ(log.props, NULL);
	ASSERT_MEM_EQ(log.props->key, "A", 2);
	ASSERT_MEM_EQ(log.props->value, "a", 2);

	prop_t* prop2 = malloc(sizeof(prop_t));
	log_set(&log, prop2, "B", "b");

	ASSERT_EQ(log.props, prop2);
	ASSERT_MEM_EQ(log.props->key, "B", 2);
	ASSERT_MEM_EQ(log.props->value, "b", 2);

	ASSERT_EQ(log.props->next, prop);
	ASSERT_MEM_EQ(log.props->next->key, "A", 2);
	ASSERT_MEM_EQ(log.props->next->value, "a", 2);

	free(prop);
	free(prop2);
	return 0;
}

int test_log_size()
{
	log_t log;
	ASSERT_EQ(init_test_log(&log), 0);
	ASSERT_EQ(log_size(&log), 1);

	log_t log2;
	log_init(&log2);
	ASSERT_EQ(log_size(&log2), 0);

	free(log.props);
	return 0;
}

int test_log_remove()
{
	log_t log;
	prop_t* removed;
	prop_t* temp;

	ASSERT_EQ(init_test_log(&log), 0);

	temp = log.props;
	log.props = malloc(sizeof(prop_t));
	log.props->key = "D";
	log.props->value = "d";
	log.props->next = temp;

	temp = log.props;
	log.props = malloc(sizeof(prop_t));
	log.props->key = "C";
	log.props->value = "c";
	log.props->next = temp;

	ASSERT_EQ(log_remove(&log, "B"), NULL);

	removed = log_remove(&log, "D");
	ASSERT_NEQ(removed, NULL);
	ASSERT_MEM_EQ(removed->value, "d", 2);
	ASSERT_NEQ(log.props, NULL);
	free(removed);

	removed = log_remove(&log, "A");
	ASSERT_NEQ(removed, NULL);
	ASSERT_MEM_EQ(removed->value, "a", 2);
	ASSERT_NEQ(log.props, NULL);
	free(removed);

	removed = log_remove(&log, "C");
	ASSERT_NEQ(removed, NULL);
	ASSERT_MEM_EQ(removed->value, "c", 2);
	ASSERT_EQ(log.props, NULL);
	free(removed);

	removed = log_remove(&log, "A");
	ASSERT_EQ(removed, NULL);

	return 0;
}

int main(int argc, char* argv[])
{
	test_ctx_t ctx;
	TEST_INIT(ctx, argc, argv);

	TEST_RUN(ctx, test_log_create);
	TEST_RUN(ctx, test_log_get);
	TEST_RUN(ctx, test_log_set);
	TEST_RUN(ctx, test_log_remove);
	TEST_RUN(ctx, test_log_size);

	TEST_RELEASE(ctx);
}
