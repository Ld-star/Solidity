pragma experimental SMTChecker;
contract C {

function f() public pure { (int[][]); }

}
// ----
// Warning: (73-82): Statement has no effect.
// Warning: (74-79): Assertion checker does not yet implement type type(int256[] memory)
// Warning: (74-79): Assertion checker does not yet implement this expression.
// Warning: (74-81): Assertion checker does not yet implement type type(int256[] memory[] memory)
// Warning: (73-82): Assertion checker does not yet implement type type(int256[] memory[] memory)
