contract test {
    function f() public {
        ufixed256x1 a = 123456781234567979695948382928485849359686494864095409282048094275023098123.5;
        ufixed256x77 b = 0.920890746623327805482905058466021565416131529487595827354393978494366605267637;
        ufixed224x78 c = 0.000000000001519884736399797998492268541131529487595827354393978494366605267646;
        fixed256x1 d = -123456781234567979695948382928485849359686494864095409282048094275023098123.5;
        fixed256x76 e = -0.93322335481643744342575580035176794825198893968114429702091846411734101080123;
        fixed256x79 g = -0.0001178860664374434257558003517679482519889396811442970209184641173410108012309;
        a; b; c; d; e; g;
    }
}
// ----
// TypeError 5107: (153-250): Type rational_const 9208...(70 digits omitted)...7637 / 1000...(71 digits omitted)...0000 is not implicitly convertible to expected type ufixed256x77, but it can be explicitly converted.
// TypeError 5107: (470-566): Type rational_const -933...(70 digits omitted)...0123 / 1000...(70 digits omitted)...0000 is not implicitly convertible to expected type fixed256x76, but it can be explicitly converted.
