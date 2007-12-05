#include <ruby.h>
#include "keyvalcfg.h"
#include <string.h>

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

struct keyval_node * keyvalcfg_node_unwrap(VALUE rb_node) {
	VALUE name, value, comment, children;
	struct keyval_node * node = calloc(1, sizeof(struct keyval_node));

	name = rb_iv_get(rb_node, "@name");
	if (name != Qnil && RSTRING_LEN(name)) {
		node->name = strdup(StringValuePtr(name));
	}

	value = rb_iv_get(rb_node, "@value");
	if (value != Qnil) {
		VALUE v = rb_funcall(value, rb_intern("to_s"), 0);
		if (RSTRING_LEN(v)) node->value = strdup(StringValuePtr(v));
	}

	comment = rb_iv_get(rb_node, "@comment");
	if (comment != Qnil && RSTRING_LEN(comment)) {
		node->comment = strdup(StringValuePtr(comment));
	}

	children = rb_iv_get(rb_node, "@children");
	if (children != Qnil) {
		int len = NUM2INT(rb_funcall(children, rb_intern("length"), 0));
		int i;
		struct keyval_node * last = NULL;
		for (i = 0; i < len; i++) {
			struct keyval_node * cur;
			VALUE child = rb_ary_entry(children, i);
			cur = keyvalcfg_node_unwrap(child);
			if (last) {
				last->next = cur;
			}
			cur->head = last ? last->head : cur;
			last = cur;
		}
		if (last) node->children = last->head;
	}

	return node;
	
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
		rb_raise(rb_eRuntimeError, "%s", error);
		free(error);
		return Qnil;
	}

	rb_node = keyvalcfg_node_wrap(node);

	keyval_node_free_all(node);
	return rb_node;
}

VALUE keyvalcfg_node_write_rb(VALUE self, VALUE path) {
	struct keyval_node * node = keyvalcfg_node_unwrap(self);
	if (keyval_write(node, path == Qnil ? 0 : StringValuePtr(path))) {
		return Qtrue;
	}

	return Qnil;
}

void Init_keyvalcfg_wrap(void) {
	keyvalcfg_module = rb_define_module("KeyvalCfg");
	keyvalcfg_node_class = rb_define_class_under(keyvalcfg_module, "Node",
	                                             rb_cObject);

	rb_define_module_function(keyvalcfg_module, "parse_file",
	                          keyvalcfg_module_parse_file, 1);
	rb_define_method(keyvalcfg_module, "parse_string",
	                 keyvalcfg_module_parse_string, 1);
	rb_define_method(keyvalcfg_node_class, "_write",
	                 keyvalcfg_node_write_rb, 1);

}
