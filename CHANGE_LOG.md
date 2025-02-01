# Changelog
All notable changes to this project/module will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project/module adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## V1.2.0 - 01.02.2025

### Added
 - Added new API functions
    + led_is_on              
    + led_set_on_brightness  
    + led_set_off_brightness 
    + led_get_duty        
 - Added minimum duty cycle option to config    

### Changed
 - LL drivers (either GPIO or TIMER) is now initializing in *led_init* function (previously only checked if they were init)
 - Duty cycle varible change unit to % (only internal change in module)

---
## V1.1.0 - 08.11.2023

### Added
 - Added implementation of get init flag function
 - Added de-init function

### Changed
 - Replace version.txt with CHANGE_LOG.md
 - Updated documentation (README.md)

### Todo
 - Separate peripheral level (timer & gpio) to interface
   file

---
## V1.0.0 - 29.09.2021

### Notice
 - Initial release

### Added
 - Configuration via single table
 - LED fading in/out
 - Runtime fading configuration change
 - Support GPIO and timer PWM low level driver

### Todo
 - Separate peripheral level (timer & gpio) to interface
   file

---
