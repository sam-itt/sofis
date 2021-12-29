# SoFIS - Open Source EFIS for your Experimental

..., Ultralight and Simulator !

SoFIS aims at providing a full EFIS experience using off the shelf components.

SoFIS is written in C, from scratch, with minimal dependencies.

Still in the early stages of development it can already show your situation and
attitude. It currently supports reading pre-recording flight data, getting
real time data from FlightGear over the network, from a [Stratux][7] unit, and
finaly from a couple of sensors.

Running a pre-recorded (circuit) flight from LFLG (Grenoble, France):

![lflg circuit][1]

Doing a direct-to from LFLG to LFMA (Aix-en-provance, France):

![ddt-lfma][9]

Controls are currently mapped to keyboard keys:
* <kbd>g</kbd> to show the dialog
* <kbd>&#8593;</kbd>, <kbd>&#8595;</kbd>, <kbd>&#8592;</kbd>, <kbd>&#8594;</kbd>: Contextual moves in the controls (scroll, change text, etc.)
* <kbd>Enter</kbd> Validate

Those controls will be mapped to a two-ring-plus-button rotatry encodre. Think
as the left/right arrow being one of the ring, up/down the other one and enter
the button click.

Mini-map current keyboard controls are as follows:
* Keypad arrow keys: Move the map
* Keypad <kbd>/</kbd>: Center the map on plane
* Keypad <kbd>+</kbd>: Zoom in
* Keypad <kbd>-</kbd>: Zoom out

Running on the very first Raspberry Pi:

![raspberry][2]

## Building from source

### Dependencies

* SDL2
* SDL2_Image
* [SDL_gpu][3]
* Glib (Only GArray and GPtrArray are really used/needed)
* libcurl
* libgps

If you don't want 3D synthetic vision you won't need glib and SDL_gpu. Just set
`ENABLE_3D=0` in switches.local

Please note that the project is still in early stages and doesn't have a
fully-fledged build system (autotools is in the works) that can detect
everything for you. You'll need to resort to `Makefile` (and `switches.local`)
editing for the time being.

### Building
```sh
$ git clone https://github.com/sam-itt/sofis
$ cd sofis
$ git submodule update --init --recursive
$ cd sdl-pcf
$ autoreconf -i
$ ./configure --with-texture=sdl_gpu
```

Here we just need configure to generate a header for sdl_gpu to be usable.
Do *not* type make to build sdl_gpu at this stage. Continue with:

```sh
$ cd ..
$ wget https://github.com/sam-itt/fg-roam/archive/media.tar.gz
$ tar -xf media.tar.gz --strip-components=1 -C fg-roam/
```

If you are building for/on the Raspberry Pi, create a `switches.local` with the
following content:

```
USE_GLES=1
NO_PRELOAD=1
TINY_TEXTURES=1
GL_LIB=brcmGLESv2
BNO080_DEV=\"/dev/i2c-0\"
```

Then, proceed with the build:
```sh
$ make
```

## Running

You can then run the self-contained demo with:

```sh
$ ./sofis --fgtape
```

SoFIS comes with pre-recorded flight data of a circuit around LFLG (Grenoble,
France).

Please note that the first run will be slower to start than others. SoFIS will
download content from FlightGear's mirrors for the synthetic vision and from
OSM + OpenAIP for the moving map. This download feature has been baked in for
convenience during testing/development. It will be disabled on "release" builds
that will rely only on pre-loaded data.

You can zoom in/out the minimap using + and - keys on the keypad and move the
minimap itself using arrow keys. Press space to toggle the synthetic vision.

## Getting data from FlightGear

SoFIS can be fed data over the network by FlightGear. You'll need to setup your
FlightGear install by copying `fg-io/flightgear-connector/basic_proto.xml` into
`$FG_ROOT/Protocol` on the FlightGear host. Once done, you can start SoFIS which
will tell you what to do:

```sh
$ ./sofis --fgremote
[...]
Waiting for first packet from FlightGear
Be sure to:
1. have basic_proto.xml in $FG_ROOT/Protocol
2. Run FlightGear(fgfs) with --generic=socket,out,5,LOCAL_IP,6789,udp,basic_proto
Be sure to replace LOCAL_IP with the IP of the local machine, one of:
	wlan0 IP Address 192.168.1.41
```

*WARNING:* Startup times on the Raspberry Pi can be *very* long, especillay with
big 3d tiles. In my tests running the [ksfo-loop][4] on a remote computer and
SoFIS in fgremote mode lead to arround *10 minutes of wall-clock* loading time
(downloading+loading the btg). If SoFIS says "Loading btg:" it's not stuck, it
iiis loading.

## Getting data from Stratux

Stratux support is is very basic and uses only the GPS and AHRS values reported
by the device. SoFIS currently currently uses the json protocol. GDL 90 support
will come later.

You need to first connect the device you'll be running SoFIS to Startux over
wifi, and the you can launch SoFIS as follows:

```sh
$ ./sofis --stratux
```

## Getting data from sensors

SoFIS is still in early stages and currently doesn't have support for many
sensors. Your patches/pull requests are very welcome!

Currently, these sensors are supported:

|Kind         | Device      | State         |
|-------------|-------------|---------------|
|GPS          | gpsd        |  OK           |
|AHRS         | BNO080      |  OK           |

### Position

SoFIS needs [gpsd][5] to be properly configured and running.
Please refer to gpsd documentation on how to do that. Once you have gpsd
correctly outputing data from your GPS receiver, you can proceed with the next
steps.

### Atittude / Heading

SoFIS currently supports the BNO080 from Bosch/Hillcrest. This unit does
onboard sensor fusion (gyros, accelerometers and magnetometer) and outputs the
resulting orientation quaternion over i2c.

If you are running on a Raspberry Pi, and using onboard i2c, you'll need to
modify your `/boot/config.txt` to get it to work. The Pi has a well-known
hardware bug that make clock stretching impossible. This is easily fixed by
setting the clock rate to a value that is acceptable to both the BNO080 and
the Pi:

```
dtparam=i2c_arm=on
dtparam=i2c_arm_baudrate=400000
```
SoFIS expects the IMU on `/dev/i2c-1` (default) or `/dev/i2c-0` (rpi). This
value can be changed in `switches.local`

### Command line

Run SoFIS with the following command line:
```sh
$ ./sofis --sensors
```

## Running on the Raspberry Pi (1/Zero)

SoFIS has been tested on the Raspberry Pi 1 model B+:


It can surely work on later models, this is just untested due to the lack of
hardware. Your patches are welcome to fix any build issues on these platforms.

SoFIS currently needs a 128/128 MB memory split which makes Raspbian really
slow. A Gentoo install is far more usable as it uses around 16MB of RAM at
logon. Here is a [link][6] to a bootable sd-card image that already have the
dependencies built in.

### Building the SD Card

Your SD Card must be at least 8GB for this image be written as-is.

*WARNING* The following instructions *can* break your system if done improperly.
Be sure of the device you are writting to: A mistake here means a wiped hard
drive. You have been warned.

We are going to assume that your sd-card is `/dev/mmcblkX`. Note that we are not
using partitions, but the whole device.

```sh
$ wget https://github.com/sam-itt/gentoo-pie/releases/download/0.0.1/sdcard-gentoo.img.xz
$ wget https://github.com/sam-itt/gentoo-pie/releases/download/0.0.1/sdcard-gentoo.img.sha1
$ sha1sum -c sdcard-gentoo.img.xz.sha1
sdcard-gentoo.img.xz: OK
# xzcat sdcard-gentoo.img.xz > /dev/mmcblkX && sync
```

You can now put the card in your Pi and boot. Login/passwords are:

* pi/pi
* root/pi

Continue through the steps at [Building](#building)

### Wiring

| BNO080      | RPI          |
|-------------|--------------|
| VCC (3.3V)  | #1 - 3.3V    |
| GND         | #6 - GND     |
| SDA         | #8 - SDA0    |
| SCL         | #9 - SCL0    |

![rpi-bno080][8]

## Contributing

SoFIS is still in very early stages of development. If you are willing to help
with the project, please have a look at the issues and the `TODO` file at the
root of the project (if there  is any) to avoid duplicate work and see where
we are currently going.

If you found a bug that you've fixed, don't hesitate to send patches or pull
requests.

If you want to improve a feature, add something or do more in-depth work, please
get in touch first by opening a new github issue.

[1]: https://github.com/sam-itt/sofis/blob/media/sofis-demo-480p.gif?raw=true
[2]: https://github.com/sam-itt/sofis/blob/media/sofis-rpi-b.png?raw=true
[3]: https://github.com/grimfang4/sdl-gpu
[4]: https://wiki.flightgear.org/Suggested_Prerecorded_Flights
[5]: https://gpsd.io/
[6]: https://github.com/sam-itt/gentoo-pie/releases/download/0.0.1/sdcard-gentoo.img.xz
[7]: http://stratux.me/
[8]: https://github.com/sam-itt/sofis/blob/media/sofis-bno080-rpi.png?raw=true
[9]: https://github.com/sam-itt/sofis/blob/media/sofis-direct-to.gif?raw=true

