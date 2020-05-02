/**
 * Copyright (c) 2017 Parrot S.A.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Parrot Company nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE PARROT COMPANY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file futils_test_list.c
 *
 * @brief list unit tests
 *
 */

#include "futils_test.h"

#define NB_TEST_ELEMENTS 10

struct list_test_element {
	int value;
	struct list_node node;
};


static void reset_list(struct list_node *list,
		       struct list_test_element *elements,
		       int count,
		       int fill)
{
	list_init(list);
	for (int i = 0; i < count; i++) {
		elements[i].value = i;
		list_node_unref(&elements[i].node);
		if (fill)
			list_add_before(list, &elements[i].node);
	}
}


static void check_walk_forward(struct list_node *list, int *tst, int tst_count)
{
	struct list_node *node;
	int counter = 0;
	list_walk_forward(list, node)
	{
		struct list_test_element *el =
			list_entry(node, struct list_test_element, node);
		CU_ASSERT(counter < tst_count);
		if (counter >= tst_count)
			return;
		CU_ASSERT_EQUAL(el->value, tst[counter]);
		counter++;
	}
	CU_ASSERT_EQUAL(counter, tst_count);
}


static void check_walk_backward(struct list_node *list, int *tst, int tst_count)
{
	struct list_node *node;
	int counter = 0;
	list_walk_backward(list, node)
	{
		struct list_test_element *el =
			list_entry(node, struct list_test_element, node);
		CU_ASSERT(counter < tst_count);
		if (counter >= tst_count)
			return;
		CU_ASSERT_EQUAL(el->value, tst[tst_count - counter - 1]);
		counter++;
	}
	CU_ASSERT_EQUAL(counter, tst_count);
}


static void
check_walk_forward_safe(struct list_node *list, int *tst, int tst_count)
{
	struct list_node *node, *tmp;
	int counter = 0;
	list_walk_forward_safe(list, node, tmp)
	{
		struct list_test_element *el =
			list_entry(node, struct list_test_element, node);
		CU_ASSERT(counter < tst_count);
		if (counter >= tst_count)
			return;
		CU_ASSERT_EQUAL(el->value, tst[counter]);
		counter++;
	}
	CU_ASSERT_EQUAL(counter, tst_count);
}


static void
check_walk_backward_safe(struct list_node *list, int *tst, int tst_count)
{
	struct list_node *node, *tmp;
	int counter = 0;
	list_walk_backward_safe(list, node, tmp)
	{
		struct list_test_element *el =
			list_entry(node, struct list_test_element, node);
		CU_ASSERT(counter < tst_count);
		if (counter >= tst_count)
			return;
		CU_ASSERT_EQUAL(el->value, tst[tst_count - counter - 1]);
		counter++;
	}
	CU_ASSERT_EQUAL(counter, tst_count);
}


static void
check_walk_entry_forward(struct list_node *list, int *tst, int tst_count)
{
	struct list_test_element *el;
	int counter = 0;
	list_walk_entry_forward(list, el, node)
	{
		CU_ASSERT(counter < tst_count);
		if (counter >= tst_count)
			return;
		CU_ASSERT_EQUAL(el->value, tst[counter]);
		counter++;
	}
	CU_ASSERT_EQUAL(counter, tst_count);
}


static void
check_walk_entry_backward(struct list_node *list, int *tst, int tst_count)
{
	struct list_test_element *el;
	int counter = 0;
	list_walk_entry_backward(list, el, node)
	{
		CU_ASSERT(counter < tst_count);
		if (counter >= tst_count)
			return;
		CU_ASSERT_EQUAL(el->value, tst[tst_count - counter - 1]);
		counter++;
	}
	CU_ASSERT_EQUAL(counter, tst_count);
}


static void
check_walk_entry_forward_safe(struct list_node *list, int *tst, int tst_count)
{
	struct list_test_element *el, *tmp;
	int counter = 0;
	list_walk_entry_forward_safe(list, el, tmp, node)
	{
		CU_ASSERT(counter < tst_count);
		if (counter >= tst_count)
			return;
		CU_ASSERT_EQUAL(el->value, tst[counter]);
		counter++;
	}
	CU_ASSERT_EQUAL(counter, tst_count);
}


static void
check_walk_entry_backward_safe(struct list_node *list, int *tst, int tst_count)
{
	struct list_test_element *el, *tmp;
	int counter = 0;
	list_walk_entry_backward_safe(list, el, tmp, node)
	{
		CU_ASSERT(counter < tst_count);
		if (counter >= tst_count)
			return;
		CU_ASSERT_EQUAL(el->value, tst[tst_count - counter - 1]);
		counter++;
	}
	CU_ASSERT_EQUAL(counter, tst_count);
}

static void check_walk(struct list_node *list, int *tst, int tst_count)
{
	check_walk_forward(list, tst, tst_count);
	check_walk_backward(list, tst, tst_count);
	check_walk_forward_safe(list, tst, tst_count);
	check_walk_backward_safe(list, tst, tst_count);
	check_walk_entry_forward(list, tst, tst_count);
	check_walk_entry_backward(list, tst, tst_count);
	check_walk_entry_forward_safe(list, tst, tst_count);
	check_walk_entry_backward_safe(list, tst, tst_count);
}


static void test_core(void)
{
	struct list_node list;
	struct list_test_element elements[NB_TEST_ELEMENTS];
	reset_list(&list, elements, NB_TEST_ELEMENTS, 0);

	/* Checks on empty list */
	CU_ASSERT_EQUAL(list_is_empty(&list), 1);
	CU_ASSERT_EQUAL(list_length(&list), 0);

	/* Fill with some data */
	for (int i = 0; i < NB_TEST_ELEMENTS; i++) {
		struct list_node *node = &elements[i].node;
		CU_ASSERT_EQUAL(list_node_is_ref(node), 0);
		CU_ASSERT_EQUAL(list_node_is_unref(node), 1);
		list_add_before(&list, node);
		CU_ASSERT_EQUAL(list_node_is_ref(node), 1);
		CU_ASSERT_EQUAL(list_node_is_unref(node), 0);
	}

	/* Non-empty list */
	CU_ASSERT_EQUAL(list_is_empty(&list), 0);
	CU_ASSERT_EQUAL(list_length(&list), NB_TEST_ELEMENTS);
	CU_ASSERT_EQUAL(list_is_last(&list, list_first(&list)), 0);
	CU_ASSERT_EQUAL(list_is_last(&list, list_last(&list)), 1);
}


static void test_modif(void)
{
	struct list_node list;
	struct list_test_element elements[NB_TEST_ELEMENTS];
	int tst[NB_TEST_ELEMENTS];
	int tst_len;
	reset_list(&list, elements, NB_TEST_ELEMENTS, 0);

	/* Add element 0 at beginning */
	list_add_after(&list, &elements[0].node);
	tst_len = 0;
	tst[tst_len++] = 0;
	check_walk_forward(&list, tst, tst_len);

	/* Add element 1 at the end */
	list_add_before(&list, &elements[1].node);
	tst_len = 0;
	tst[tst_len++] = 0;
	tst[tst_len++] = 1;
	check_walk_forward(&list, tst, tst_len);

	/* Add element 2 after 0 */
	list_add_after(&elements[0].node, &elements[2].node);
	tst_len = 0;
	tst[tst_len++] = 0;
	tst[tst_len++] = 2;
	tst[tst_len++] = 1;
	check_walk_forward(&list, tst, tst_len);
	/* Add element 3 before element 2 */
	list_add_before(&elements[2].node, &elements[3].node);
	tst_len = 0;
	tst[tst_len++] = 0;
	tst[tst_len++] = 3;
	tst[tst_len++] = 2;
	tst[tst_len++] = 1;
	check_walk_forward(&list, tst, tst_len);

	/* Add element 4 between element 0 and 1 (removing
	 * elements 2 and 3) */
	list_add(&elements[4].node, &elements[0].node, &elements[1].node);
	tst_len = 0;
	tst[tst_len++] = 0;
	tst[tst_len++] = 4;
	tst[tst_len++] = 1;
	check_walk_forward(&list, tst, tst_len);

	/* Move element 0 after element 1 */
	list_move_after(&elements[1].node, &elements[0].node);
	tst_len = 0;
	tst[tst_len++] = 4;
	tst[tst_len++] = 1;
	tst[tst_len++] = 0;
	check_walk_forward(&list, tst, tst_len);

	/* Move element 0 before element 1 */
	list_move_before(&elements[1].node, &elements[0].node);
	tst_len = 0;
	tst[tst_len++] = 4;
	tst[tst_len++] = 0;
	tst[tst_len++] = 1;
	check_walk_forward(&list, tst, tst_len);

	/* Replace element 4 with element 5 */
	list_replace(&elements[4].node, &elements[5].node);
	tst_len = 0;
	tst[tst_len++] = 5;
	tst[tst_len++] = 0;
	tst[tst_len++] = 1;
	check_walk_forward(&list, tst, tst_len);

	/* Detach list around element 0 */
	list_detach(elements[0].node.prev, elements[0].node.next);
	tst_len = 0;
	tst[tst_len++] = 5;
	tst[tst_len++] = 1;
	check_walk_forward(&list, tst, tst_len);

	/* Del element 5 */
	list_del(&elements[5].node);
	tst_len = 0;
	tst[tst_len++] = 1;
	check_walk_forward(&list, tst, tst_len);
}


static void test_iter(void)
{
	struct list_node list, *tmp;
	struct list_test_element elements[NB_TEST_ELEMENTS];
	int tst[NB_TEST_ELEMENTS];
	reset_list(&list, elements, NB_TEST_ELEMENTS, 0);

	/* Empty list tests */
	CU_ASSERT_EQUAL(list_is_empty(&list), 1);
	CU_ASSERT_EQUAL(list_length(&list), 0);
	check_walk(&list, NULL, 0);

	/* Fill with elements in order */
	reset_list(&list, elements, NB_TEST_ELEMENTS, 1);
	for (int i = 0; i < NB_TEST_ELEMENTS; i++)
		tst[i] = i;
	CU_ASSERT_EQUAL(list_is_empty(&list), 0);
	CU_ASSERT_EQUAL(list_length(&list), NB_TEST_ELEMENTS);
	check_walk(&list, tst, NB_TEST_ELEMENTS);
	tmp = &list;
	int count = 0;
	while ((tmp = list_next(&list, tmp)) != NULL) {
		struct list_test_element *el =
			list_entry(tmp, struct list_test_element, node);
		CU_ASSERT_EQUAL(el->value, count);
		count++;
	}
	tmp = &list;
	count = NB_TEST_ELEMENTS - 1;
	while ((tmp = list_prev(&list, tmp)) != NULL) {
		struct list_test_element *el =
			list_entry(tmp, struct list_test_element, node);
		CU_ASSERT_EQUAL(el->value, count);
		count--;
	}
}


static void test_iter_safe(void)
{
	struct list_node list;
	struct list_test_element elements[NB_TEST_ELEMENTS];

	struct list_node *node, *ntmp;
	struct list_test_element *el, *etmp;

	/* Remove elements when iterating */
	reset_list(&list, elements, NB_TEST_ELEMENTS, 1);
	list_walk_forward_safe(&list, node, ntmp)
	{
		list_del(node);
	}
	CU_ASSERT_EQUAL(list_is_empty(&list), 1);

	reset_list(&list, elements, NB_TEST_ELEMENTS, 1);
	list_walk_backward_safe(&list, node, ntmp)
	{
		list_del(node);
	}
	CU_ASSERT_EQUAL(list_is_empty(&list), 1);

	reset_list(&list, elements, NB_TEST_ELEMENTS, 1);
	list_walk_entry_forward_safe(&list, el, etmp, node)
	{
		list_del(&el->node);
	}
	CU_ASSERT_EQUAL(list_is_empty(&list), 1);

	reset_list(&list, elements, NB_TEST_ELEMENTS, 1);
	list_walk_entry_backward_safe(&list, el, etmp, node)
	{
		list_del(&el->node);
	}
	CU_ASSERT_EQUAL(list_is_empty(&list), 1);
}


static void test_fifo(void)
{
	struct list_node list;
	struct list_test_element elements[NB_TEST_ELEMENTS];
	reset_list(&list, elements, NB_TEST_ELEMENTS, 0);

	/* check push/pop to be in order */
	for (int i = 0; i < NB_TEST_ELEMENTS; i++) {
		list_push(&list, &elements[i].node);
		CU_ASSERT_EQUAL(list_length(&list), i + 1);
	}
	for (int i = 0; i < NB_TEST_ELEMENTS; i++) {
		struct list_test_element *el =
			list_pop(&list, struct list_test_element, node);
		CU_ASSERT_EQUAL(list_length(&list), NB_TEST_ELEMENTS - i - 1);
		CU_ASSERT_EQUAL(el->value, i);
	}
	/* pop from an empty fifo */
	CU_ASSERT_EQUAL(list_pop(&list, struct list_test_element, node), NULL);
}


CU_TestInfo s_list_tests[] = {
	{(char *)"list core", &test_core},
	{(char *)"list insertion/deletion", &test_modif},
	{(char *)"list iterators", &test_iter},
	{(char *)"list safe iterators", &test_iter_safe},
	{(char *)"list as a fifo", &test_fifo},
	CU_TEST_INFO_NULL,
};
