# JCU-CC2511-Assignment-2-Group-1
Repo for CC2511 embedded systems design, assignment 2 for group 1

Group members: Liam Eime, Darcy Kemp, Justin Fagg, Tahlia Clennar.

The reposiotry will contain:
  - any programs written for this assingment
  - Altium designer work

For anyone (probably CC2511 students from 2023 onwards) who wishes to use the material provided in thsi repository please make note of the followng:
  - In the hardware, a design choice was to break out the voltage divider in the DRV8825 stepper motor controllers to a single pair of headers.
    This allows for resistor values to manually be placed in the headers.
  - Also, please make note of the first comments in main.c abou the term "debug", this project was designed around not using a second PICO board to run the program
    in debug mode. The hardware is designed as such that there are header pins available to easily wire to a breaboarded second PICO board for debugging
  - It should also be noted that an alternative to the current program is to use interrupt handling and uart to execute commands and send over the COM port connection
