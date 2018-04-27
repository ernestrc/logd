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

static int init_test_log(log_t* log, str_t* key, str_t* value)
{
	log_init(log);

	*key = MAKE_STR("A");
	*value = MAKE_STR("a");

	ASSERT_EQ(log_get(log, "A", 1), NULL);

	log->props = malloc(sizeof(prop_t));
	log->props->key = *key;
	log->props->value = *value;
	log->props->next = NULL;

	return 0;
}

int test_log_get()
{
	log_t log;
	str_t key;
	str_t value;

	ASSERT_EQ(init_test_log(&log, &key, &value), 0);

	str_t expected = log_get(&log, "A", 1);
	ASSERT_NEQ(expected, NULL);
	ASSERT_MEM_EQ(LOG_GET_STRVALUE(expected), LOG_GET_STRVALUE(value),
	  LOG_GET_STRLEN(value));

	expected = log_get(&log, "B", 1);
	ASSERT_EQ(expected, NULL);

	free(log.props);
	return 0;
}

int test_log_set()
{
	log_t log;

	str_t key = MAKE_STR("A");
	str_t value = MAKE_STR("a");
	prop_t* prop = malloc(sizeof(prop_t));

	log_set(&log, prop, key, value);

	ASSERT_NEQ(log.props, NULL);
	ASSERT_EQ(log.props->key, key);
	ASSERT_EQ(log.props->value, value);

	str_t key2 = MAKE_STR("B");
	str_t value2 = MAKE_STR("b");
	prop_t* prop2 = malloc(sizeof(prop_t));
	log_set(&log, prop2, key2, value2);

	ASSERT_EQ(log.props, prop2);
	ASSERT_EQ(log.props->key, key2);
	ASSERT_EQ(log.props->value, value2);

	ASSERT_EQ(log.props->next, prop);
	ASSERT_EQ(log.props->next->key, key);
	ASSERT_EQ(log.props->next->value, value);

	free(prop);
	free(prop2);
	return 0;
}

int test_log_remove()
{
	log_t log;
	str_t key;
	str_t value;
	prop_t* removed;
	prop_t* temp;

	ASSERT_EQ(init_test_log(&log, &key, &value), 0);

	temp = log.props;
	log.props = malloc(sizeof(prop_t));
	log.props->key = MAKE_STR("D");
	log.props->value = MAKE_STR("d");
	log.props->next = temp;

	temp = log.props;
	log.props = malloc(sizeof(prop_t));
	log.props->key = MAKE_STR("C");
	log.props->value = MAKE_STR("c");
	log.props->next = temp;

	ASSERT_EQ(log_remove(&log, "B", 1), NULL);

	removed = log_remove(&log, "D", 1);
	ASSERT_NEQ(removed, NULL);
	ASSERT_MEM_EQ(LOG_GET_STRVALUE(removed->value), "d", 1);
	ASSERT_NEQ(log.props, NULL);
	free(removed);

	removed = log_remove(&log, "A", 1);
	ASSERT_NEQ(removed, NULL);
	ASSERT_MEM_EQ(LOG_GET_STRVALUE(removed->value), "a", 1);
	ASSERT_NEQ(log.props, NULL);
	free(removed);

	removed = log_remove(&log, "C", 1);
	ASSERT_NEQ(removed, NULL);
	ASSERT_MEM_EQ(LOG_GET_STRVALUE(removed->value), "c", 1);
	ASSERT_EQ(log.props, NULL);
	free(removed);

	removed = log_remove(&log, "A", 1);
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

	TEST_RELEASE(ctx);
}
