# Lcd-Dash
Small info display for PC


made using Arduino nano and python and is fetching real time info from the computer. 
The thing you see behind it is a LDR with some hot glue on top to act as a ambient light sensor. 
The measured brightness is sent to pc to adjust monitor's brightness and olso the backlight of the lcd is adjusted with the same. 
It's using serial over usb for communicating with the pc. 
The double bars are rendered using Progressbardouble library. 

Currently only working in Linux.

![Defalt Screen](%5Benter%20image%20description%20here%5D%28https://github.com/P-rth/Lcd-Dash/blob/main/Image.jpg?raw=true%29)

[Click here to see working](https://www.reddit.com/r/arduino/comments/18ctvnx/look_what_i_made_lcd_info_panel_for_pc_16x2/)



    Curcuit :
    
    A7 --> 5.6k ohms --> Groud
    Reset --> 22uf capacitor --> switch -- Groud  (turn off to upload code)
    
    LDR (+) --> Vin / 5v
    LDR (-) --> A7
    
    Backlight + (LCD A) --> D10
    Backlight - (LCD K) --> Groud
    
    
    LCD RS --> 12
    LCD Enable --> 11
    LCD D4 --> 5
    LCD D5 --> 4
    LCD D6 --> 3
    LCD D7 --> 2
