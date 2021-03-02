# README #

## CHECKLIST ##

1. `make makesum`
2. `make checksum`
3. `make extract`
4. `make build`
5. `make fake`
6. `make update-plist`
7. `make package` (`PKG_CREATE_NO_CHECKS=Yes`)
  * install
  * uninstall
8. `make port-lib-depends-check`
9. `/usr/ports/infrastructure/bin/portcheck`

## MAKE A PATCH FOR A FILE ##

The sequence to make a patch for a file is usually:

- `cd $(make show=WRKSRC)`
- `cp foo/bar.c{,.orig}`
- Edit `foo/bar.c`
- `cd -`
- `make update-patches`

(This will create `patches/patch-foo_bar_c` with your modifications.)

The easiest way to reset the port and test your patches is `make clean
patch`. This will delete the work directory, re-extract, and patch
your port.
