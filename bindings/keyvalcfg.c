#include <ruby.h>
#include "keyvalcfg.h"

static VALUE keyvalcfg_module;
static VALUE keyvalcfg_node_class;

VALUE keyvalcfg_node_wrap(struct keyval_node * node) {
	struct keyval_node * child;

	VALUE rb_node = rb_class_new_instance(0, NULL, keyvalcfg_node_class);
	VALUE rb_node_children = rb_ary_new();
	VALUE value = Qnil;

	if (node->name) rb_iv_set(rb_node, "@name", rb_str_new2(node->name));

	switch (keyval_node_get_value_type(node)) {
		case KEYVAL_TYPE_BOOL:
			value = keyval_node_get_value_bool(node) ? Qtrue : Qnil;
			break;
		case KEYVAL_TYPE_INT:
			value = INT2NUM(keyval_node_get_value_int(node));
			break;
		case KEYVAL_TYPE_DOUBLE:
			value = rb_float_new(keyval_node_get_value_double(node));
			break;
		case KEYVAL_TYPE_STRING:
			value = rb_str_new2(node->value);
			break;
		default: break;
	}

	rb_iv_set(rb_node, "@value", value);
	if (node->comment) rb_iv_set(rb_node, "@comment", rb_str_new2(node->comment));

	for (child = node->children; child; child = child->next) {
		rb_ary_push(rb_node_children, keyvalcfg_node_wrap(child));
	}

	rb_iv_set(rb_node, "@children", rb_node_children);

	return rb_node;
}

VALUE keyvalcfg_module_parse_file(VALUE module, VALUE path) {
	VALUE rb_node;
	struct keyval_node * node = keyval_parse_file(StringValuePtr(path));
	char * error = keyval_get_error();
	if (error) {
		rb_raise(rb_eRuntimeError, "%s", error);
		free(error);
		return Qnil;
	}

	rb_node = keyvalcfg_node_wrap(node);

	keyval_node_free_all(node);
	return rb_node;
}

VALUE keyvalcfg_module_parse_string(VALUE module, VALUE string) {
	VALUE rb_node;
	struct keyval_node * node = keyval_parse_string(StringValuePtr(string));
	char * error = keyval_get_error();
	if (error) {
		return Qnil;
	}

	rb_node = keyvalcfg_node_wrap(node);

	keyval_node_free_all(node);
	return rb_node;
}

void Init_keyvalcfg(void) {
	rb_require("keyvalcfg_wrap");

	keyvalcfg_module = rb_define_module("KeyvalCfg");
	keyvalcfg_node_class = rb_define_class_under(keyvalcfg_module, "Node",
	                                             rb_cObject);

	rb_define_module_function(keyvalcfg_module, "parse_file",
	                          keyvalcfg_module_parse_file, 1);
	rb_define_method(keyvalcfg_module, "parse_string",
	                 keyvalcfg_module_parse_string, 1);

}
