abstract contract A {
	int public testvar;
	function foo() internal returns (uint256);
}
abstract contract B {
	function foo() internal returns (uint8);
	function test() internal returns (uint256);
}
abstract contract X is A, B {
	int public override testvar;
	function test() internal override returns (uint256);
}
// ----
// TypeError: (200-315): Derived contract must override function "foo". Function with the same name and parameter types defined in two or more base classes.
