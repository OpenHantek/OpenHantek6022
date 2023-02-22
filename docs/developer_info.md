# OpenHantek developer documentation

This document explains the basic data flow and concepts used and should be amended
whenever a new concept is added to the code base.
Detailed information about an implementation are to be found in the specific directory readmes though.

## Directory structure

The *res* folder contains mostly binary resource data that are bundled with the executable,
like icons, images, firmwares and translations.

The *res/translations* folder contains translations in the Qt format. Use Qt linguist
as a handy graphical interface to translate OpenHantek. A [Translation HowTo](../openhantek/translations/Translation_HowTo.md) is available.

The source code within *src* can be divided into a **core**, that is responsible for device communication,
data acquisition and post processing and the **graphical interface** with several custom widgets,
a configuration interface and an OpenGL renderer.

### Core structure

The raw device communication takes place in the *src/usb* directory, especially via the `USBDevice` class.
To find suitable devices, the `FindDevices` class in the same folder is used. Firmware upload is realized
via the `ezusb` helper methods and the `UploadFirmware` class.

The hantek protocol structures and constants are defined within `src/hantekprotocol`.

The heart of OpenHantek is the `src/hantekdso` folder and its `hantekdsocontrol` class. All supported models, 
based on the `DsoModel` class, are implemented within a subfolder `src/hantekdso/models` and automatically register
themself to a `ModelRegistry` class. A model is based on (contains) a specification, the `ControlSpecification` class.

The `hantekdsocontrol` class keeps track of the devices current state (samplerate, selected gain, activated channels,
etc) via the `ControlSettings` class and field.
It outputs the channel separated unprocessed samples via a `samplesAvailable(DSOsamples*)` signal.

Before the data is presented to the GUI it arrives in the `src/post/postprocessing` class. Several post
processing classes are to be found in this directory as well.

### Graphical interface structure

The initial dialog for device selection is realized in *src/selectdevice* where several models
and dialogs are implemented. This is basically a graphical wrapper around the `src/usb/finddevices` class.

You will find the configuration dialog pages to be implemented in *src/configdialog*.

Custom widgets like a LevelSlider with a unit suffix reside in *src/widgets*, the custom main window docks are
in *src/docks*.

The code that is responsible for exporting data to images or to the printer is stored in *src/exporting*.

The main window itself doesn't do and shouldn't do much more than connecting signals/slots between the core part
and the graphical part.

All OpenGL rendering takes place in the `GlScope` class. A helper class `GlScopeGraph` contains exactly one
data sample snapshot including all channels for voltage and spectrum and a pointer to the respective GPU buffer.
`GlScope` works normally for **OpenGL 3.2+** and OpenGL ES 2.0+ but if it detects **OpenGL 2.1+** and OpenGL ES 1.2+ on older platforms it switches to a legacy implementation. If both OpenGL and OpenGL Es are present, OpenGL will be preferred, but can be overwritten by the user via a command flag.

### Export

All export related functionality is within *src/exporting*.

The following exporters are implemented:

* Export to comma separated value file (CSV): Write to a user selected file,

All export classes (at the moment only exportcsv) implement the
ExporterInterface and are registered to the ExporterRegistry in the main.cpp.

Screen shot / hard copy is realised by converting the screen content to PNG or PDF format.
So you will get exactly the content of the screen.
For hard copy the screen colors are adapted shortly to have dark traces on white background output.

## Persistent settings

Persisent settings for individual devices are stored in files like  `~/.config/OpenHantek/DSO-6022BE_123456789ABC.conf`,
where the filename is combined from device type and unique serial number. This allows to work with more than one scope at the same time.
Settings are written at program shutdown or with menu entry `File/Save settings` (`Ctrl-S`).
The save/load happens in `src/dsosettings.cpp` in the functions `DsoSettings::save()` and `DsoSettings::load()`.
After loading of the settings the values will be applied to the internal status as well as to the GUI.

An example of making the calibration output frequency (parameter `calfreq`) persistent can be seen in commit [26c1a49](https://github.com/OpenHantek/OpenHantek6022/commit/26c1a49146818aab7419b319705878a5f072460f)

`openhantek/src/dsosettings.cpp`

    void DsoSettings::save() {
        ...
	   store->setValue("calfreq", scope.horizontal.calfreq);
	   ...
    }

    void DsoSettings::load() {
        ...
	   if (store->contains("calfreq")) scope.horizontal.calfreq = store->value("calfreq").toDouble();
	   ...
    }

`openhantek/src/hantekdso/hantekdsocontrol.cpp`

    void HantekDsoControl::applySettings(DsoSettingsScope *scope) {
        ...
	   setCalFreq(scope->horizontal.calfreq);
	   ...
    }

## Scope interface

The scope receives control commands via endpoint 0 and provides the sampled data via bulk transfer on endpoint 6.
The firmware command parser:

    BOOL handle_vendorcommand(BYTE cmd)
    {
     stop_sampling();

     /* Set red LED, toggle after timeout. */
	LED_RED();
	ledcounter = ledinit;

     /* Clear EP0BCH/L for each valid command. */
	if ( cmd >= 0xe0 && cmd <= 0xe6 ) {
	     EP0BCH = 0;
		EP0BCL = 0;
		while ( EP0CS & bmEPBUSY )
		     ;
			}

     switch (cmd) {
          case 0xa2:
		     return eeprom();

          case 0xe0:
		     return set_voltage( 0, EP0BUF[0] );

          case 0xe1:
		     return set_voltage( 1, EP0BUF[0] );

          case 0xe2:
		     return set_samplerate( EP0BUF[0] );

          case 0xe3:
		     if ( EP0BUF[0] == 1 ) {
			         /* Set green LED while sampling. */
				    LED_GREEN();
				    ledcounter = 0;
				    start_sampling();
				}
			return TRUE;

          case 0xe4:
		     return set_numchannels( EP0BUF[0] );

          case 0xe5:
		     SET_COUPLING( EP0BUF[0] );
			return TRUE;

          case 0xe6:
		     return set_calibration_pulse( EP0BUF[0] );

     }

     return FALSE; /* Not handled by handlers. */
    }


## Data flow
* The procedure `void HantekDsoControl::stateMachine()` controls the raw data capturing, conversion to real world physical values, trigger detection and timing of screen refresh. The `struct Raw` holds all important values of one sampled data block:


    struct Raw {
        unsigned channels = 0;
        double samplerate = 0;
        unsigned oversampling = 0;
        unsigned gainValue[ 2 ] = {1, 1}; // 1,2,5,10,..
        unsigned gainIndex[ 2 ] = {7, 7}; // index 0..7
        unsigned tag = 0;
        bool freeRun = false;  // small buffer, no trigger
        bool valid = false;    // samples can be processed
        bool rollMode = false; // one complete buffer received, start to roll
        unsigned size = 0;
        unsigned received = 0;
        std::vector< unsigned char > data;
        mutable QReadWriteLock lock;
    };


* Raw 8-bit ADC values are collected permanently via call to `HantekDsoControl::getSamples(..)` (or `...getDemoSamples(..)` for the demo device) in an own thread `Capturing::Capturing()`.
At fast sample rates (>= 10 kS/s) one big block is requested via USB command to make the transfer more robust against USB interruptions by other traffic, 
while at slow sample rates it requests the data in small chunks to allow a permanent screen update in roll mode.
* Raw values are converted in `HantekDsoControl::convertRawDataToSamples()` to real-world double samples (scaled with voltage and sample rate). 
The 2X..200X oversampling for slower sample rates is done here. Also overdriving of the inputs is detected.
In `Roll` mode the latest sample values are always put at the end of the result buffer while older samples move toward the beginning of the buffer,
this rolls the displayed trace permanently to the left.
The conversion uses either the factory calibration values from EEPROM or from a user supplied config file. 
Read more about [calibration](https://github.com/Ho-Ro/Hantek6022API/blob/main/README.md#create-calibration-values-for-openhantek).
* `searchTriggeredPosition()`
    * Checks if the signal is triggered and calculates the starting point for a stable display.
    The time distance to the following opposite slope is measured and displayed as pulse width in the top row.
* `provideTriggeredData()` handles the trigger mode:
    * If the **trigger condition is false** and the **trigger mode is Normal** or the display is paused
then we reuse the last triggered samples so that voltage and spectrum traces
as well as the measurement at the scope's bottom lines are frozen until the trigger condition
becomes true again. The reused samples are emitted at lower speed (every 20 ms) to reduce CPU load but be responsive to user actions.
    * If the **trigger condition is false** and the **trigger mode is not Normal** then we display a free running trace and discard the last saved samples.
* The converted `DSOsamples` are emitted to PostProcessing::input() via signal/slot:

`QObject::connect( &dsoControl, &HantekDsoControl::samplesAvailable, &postProcessing, &PostProcessing::input );`


    struct DSOsamples {
        std::vector< std::vector< double > > data; ///< Pointer to input data from device
        double samplerate = 0.0;                   ///< The samplerate of the input data
        unsigned char clipped = 0;                 ///< Bitmask of clipped channels
        bool liveTrigger = false;                  ///< live samples are triggered
        unsigned triggeredPosition = 0;            ///< position for a triggered trace, 0 = not triggered
        double pulseWidth1 = 0.0;                  ///< width from trigger point to next opposite slope
        double pulseWidth2 = 0.0;                  ///< width from next opposite slope to third slope
        bool freeRunning = false;                  ///< trigger: NONE, half sample count
        unsigned tag = 0;                          ///< track individual sample blocks (debug support)
        mutable QReadWriteLock lock;
    };


* PostProzessing calls all processors that were registered in `main.cpp`.
  * `MathchannelGenerator::process()`
    * which creates a third MATH channel as one of these data sample combinations: 
      `CH1 + CH2`, `CH1 - CH2`, `CH2 - CH1`, `CH1 * CH2`, `CH1 AC` or `CH2 AC`.
  * `SpectrumGenerator::process()`
    * For each active channel:∘
      * Calculate the peak-to-peak, DC (average), AC (rms) and effective value ( sqrt( DC² + AC² ) ).
      * Apply a user selected window function and scale the result accordingly.
      * Calculate the spectrum of the AC part of the signal scaled as dBV. FFT: f(t) ∘⎯ F(ω)
      * Calculate the autocorrelation to get the frequency of the signal:
        * Calculate power spectrum |F(ω)|² and do an IFFT: F(ω) ∙ F(ω) ⎯∘ f(t) ⊗ f(t) (autocorrelation, i.e. convolution of f(t) with f(t))
        * This is quite inaccurate at high frequencies. In these cases the first peak value of the spectrum is used.
      * Calculate the THD (optional): `THD = sqrt( power_of_harmonics / power_of_fundamental )`
  * `GraphGenerator::process()`
    * which works either in TY mode and creates two types of traces:
      * voltage over time `GraphGenerator::generateGraphsTYvoltage()`
      * spectrum over frequency `GraphGenerator::generateGraphsTYspectrum()`
    * or in XY mode and creates a voltage over voltage trace `GraphGenerator::generateGraphsXY()`.
    * `GraphGenerator::generateGraphsTYvoltage()` creates up to three (CH1, CH2, MATH) voltage traces.
    The procedure takes care of interpolating in *Step* or *Sinc* mode and it will also create the histogram if enabled.
    * `GraphGenerator::generateGraphsTYspectrum()` creates up to three (SP1, SP2, SPM) spectral traces.
  * Finally `PostProcessing` emits the signal `processingFinished()` that is connected to:
    * `ExporterRegistry::input()` that takes care of exporting to CSV or JSON data.
    * `MainWindow::showNewData()`.
      * which calls `DsoWidget::showNew()` that calls `GlScope::showData()` that calls `Graph::writeData()`.

t.b.c.

## Data structures and configuration

* Two main structures/classes hold most of the current status of the oscilloscope:

* The struct `ControlSettings` and its substructures (-> `hantekdso/controlsettings.h`) are the main storage for all scope (HW) parameters that are handled by the class `HantekDsoControl`.


    struct ControlSettings {
        ControlSettings( const ControlSamplerateLimits *limits, size_t channelCount );
        ~ControlSettings();
        ControlSettings( const ControlSettings & ) = delete;
        ControlSettings operator=( const ControlSettings & ) = delete;
        ControlSettingsSamplerate samplerate;          ///< The samplerate settings
        std::vector< ControlSettingsVoltage > voltage; ///< The amplification settings
        ControlSettingsTrigger trigger;                ///< The trigger settings
        RecordLengthID recordLengthId = 1;             ///< The id in the record length array
        unsigned channelCount = 0;                     ///< Number of activated channels
        Hantek::CalibrationValues *calibrationValues;  ///< Calibration data for the channel offsets & gains
        Hantek::ControlGetLimits cmdGetLimits;
    };

* The class `DsoSettings` (-> `dsosettings.h`) and its substructures `DsoSettingsScope` (`scopesettings.h`) and `DsoSettingsView` (`viewsettings.h`)
are the main storage for all persistent scope (program) parameters, see `DsoSettings::save()` and `DsoSettings::load()`.


    class DsoSettings {
        Q_DECLARE_TR_FUNCTIONS( DsoSettings )
      public:
        explicit DsoSettings( const Dso::ControlSpecification *deviceSpecification );
        bool setFilename( const QString &filename );

        DsoSettingsExport exporting;    ///< General options of the program
        DsoSettingsScope scope;         ///< All oscilloscope related settings
        DsoSettingsView view;           ///< All view related settings
        DsoSettingsPostProcessing post; ///< All post processing related settings
        bool alwaysSave = true;         ///< Always save the settings on exit

        QByteArray mainWindowGeometry; ///< Geometry of the main window
        QByteArray mainWindowState;    ///< State of docking windows and toolbars

        /// \brief Read the settings from the last session or another file.
        void load();

        /// \brief Save the settings to the harddisk.
        void save();

      private:
        std::unique_ptr< QSettings > store = std::unique_ptr< QSettings >( new QSettings );
        const Dso::ControlSpecification *deviceSpecification;
    };

GUI input either in the docks or by moving sliders changes the `DsoSettings` parameters directly
and the `ControlSettings` parameters (that live in another thread `dsoControlThread` ) via signal/slot mechanism.
The (big) class `HantekDsoControl` has a member `const DsoSettingsScope *scope` that gives direct read access to the (persistent) scope settings.
