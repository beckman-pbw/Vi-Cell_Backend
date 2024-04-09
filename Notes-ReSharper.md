 * Template methods in headers using `MODULENAME` static constant
    * May not work with other compilers.
    * Should use some other kind of constant string.
 * `override` with `virtual`
    * We want *both*. It seems that the *same* ReSharper lint suggests
       `override` *and* declares that `virtual` is redundant, so this may not
       be possible to configure as we wish.
 * "call to a virtual function inside a destructor will be statically resolved"
    * ex: StageController calling `Quit()`.
    * Resolution: mark the method being called `final`.
    * Alternatively, examine *why* the method is `virtual` in the first place.
      If cleanup needs to be done in the destructor, methods should be
      non-virtual.
 * `if` with unreachable code: if it's error handling, use `static_assert` instead
