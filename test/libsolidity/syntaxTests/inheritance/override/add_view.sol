contract B { function f() virtual public {} }
contract C is B { function f() public view {} }
// ----
// TypeError 9456: (64-91): Overriding function is missing "override" specifier.
// TypeError 6959: (64-91): Overriding function changes state mutability from "nonpayable" to "view".
