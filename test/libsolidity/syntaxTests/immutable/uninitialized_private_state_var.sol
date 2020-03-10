contract B {
    uint immutable private x;

    constructor() public {
    }

    function f() internal view virtual returns(uint) { return 1; }
    function readX() internal view returns(uint) { return x; }
}

contract C is B {
    uint immutable y;
    constructor() public {
        y = 3;
    }
    function f() internal view override returns(uint) { return readX(); }

}
// ----
// TypeError: (0-209): Construction control flow ends without initializing all immutable state variables.
// TypeError: (211-375): Construction control flow ends without initializing all immutable state variables.
