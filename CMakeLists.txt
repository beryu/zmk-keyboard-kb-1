# The minimal hardware-test build intentionally omits the board-specific
# split power-management source so USB and the key matrix can be tested alone.
target_sources(app PRIVATE src/usb_self_test.c)
