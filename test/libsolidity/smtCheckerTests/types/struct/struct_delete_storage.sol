pragma experimental SMTChecker;
pragma experimental ABIEncoderV2;

contract C {
	struct S {
		uint x;
		uint[] a;
	}
	S s1;
	function g(S memory s2) public {
		s1.x = s2.x;
		s1.a = s2.a;
	}
	function f(S memory s2) public {
		delete s1;
		assert(s1.x == s2.x);
		assert(s1.a.length == s2.a.length);
		assert(s1.a.length == 0);
	}
}
// ----
// Warning 6328: (240-260): Assertion violation happens here
// Warning 6328: (264-298): Assertion violation happens here
