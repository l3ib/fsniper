$cases = [
	["list-not-closed-1.cfg", "keyval: error: list `unclosed` never closed (declared on line 1)"],
	["list-not-closed-2.cfg", "keyval: error: list `unclosed` never closed (declared on line 1)"],
	["list-not-closed-3.cfg", "keyval: error: list `odd numbers` never closed (declared on line 18)"],
	["no-value-1.cfg", "keyval: error: expected value after `x =` on line 1"],
	["no-value-2.cfg", "keyval: error: expected value after `this parser rocks =` on line 21"],
	["no-value-3.cfg", "keyval: error: unexpected end of line on line 8\nkeyval: (in section `int test_multiline(struct keyval_node * cfg)`)"],
	["not-closed-1.cfg", "keyval: error: section `x` never closed (declared on line 1)"],
	["not-closed-2.cfg", "keyval: error: section `x` never closed (declared on line 19)"],
	["not-closed-3.cfg", "keyval: error: section `x` never closed (declared on line 19)"]
]
