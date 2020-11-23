==== Source: A ====
pragma abicoder               v2;

struct Item {
    uint x;
}

library L {
    event Ev(Item);
}

contract C {
    function foo() public {
        emit L.Ev(Item(1));
    }
}
==== Source: B ====
import "A";

contract D is C {}
// ----
