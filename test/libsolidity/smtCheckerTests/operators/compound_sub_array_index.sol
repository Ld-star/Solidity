pragma experimental SMTChecker;

contract C
{
	uint[] array;
	function q() public { array.push(); }
	function f(uint x, uint p) public {
		require(p < array.length);
		require(x < 100);
		array[p] = 200;
		array[p] -= array[p] - x;
		assert(array[p] >= 0);
		assert(array[p] < 90);
	}
}
// ====
// SMTIgnoreCex: yes
// ----
// Warning 6328: (259-280): CHC: Assertion violation happens here.
