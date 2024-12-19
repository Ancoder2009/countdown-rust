#![no_std]
#![no_main]

use arduino_hal::prelude::*;
use ds323x::{Ds323x, Rtcc, Timelike};
use panic_halt as _;

#[arduino_hal::entry]
fn main() -> ! {
    let dp = arduino_hal::Peripherals::take().unwrap();
    let pins = arduino_hal::pins!(dp);
    let mut serial = arduino_hal::default_serial!(dp, pins, 57600);

    let i2c = arduino_hal::I2c::new(
        dp.TWI,
        pins.a4.into_pull_up_input(),
        pins.a5.into_pull_up_input(),
        50000,
    );

    ufmt::uwriteln!(&mut serial, "Hello World!\n").unwrap_infallible();

    let mut rtc = Ds323x::new_ds3231(i2c);

    loop {
        ufmt::uwriteln!(&mut serial, "{}", rtc.time().unwrap().second()).unwrap_infallible();
    }
}
