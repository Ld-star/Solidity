contract C {
    struct S { bool f; }
    S s;
    function f() internal pure {
        S storage c;
        // this should warn about unreachable code, but currently function flow is ignored
        assembly {
            function f() { return(0, 0) }
            f()
            c_slot := s_slot
        }
        c;
    }
}
// ----
