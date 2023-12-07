import time
import datetime
import threading
import queue
import numpy as np
import os
import psutil
import serial
import Xlib.display
import dbus
import time
import threading



class ScrollingText:
    def __init__(self, limit=16, speed=0.1):
        self.text = ""
        self.limit = limit
        self.speed = speed
        self.running = False
        self.scroll_thread = None
        self.out = ''

    def update_text(self, new_text):
        self.text = new_text
        if self.running:
            # Restart the scrolling thread with the updated text
            self.stop_scrolling()
            self.start_scrolling()

    def start_scrolling(self):
        self.running = True
        self.scroll_thread = threading.Thread(target=self.scrolling_thread)
        self.scroll_thread.start()

    def stop_scrolling(self):
        self.running = False
        if self.scroll_thread:
            self.scroll_thread.join()

    def scrolling_thread(self):
        self.text = f'{self.text}   {self.text[0:16]}' 
        text_length = len(self.text)
        range_len = text_length - self.limit + 1

        while self.running:
            
            for i in range(range_len):
                time.sleep(self.speed)
                if not self.running:
                    break
                self.out = self.text[i:i + self.limit]



def get_currently_playing():
    try:
        session_bus = dbus.SessionBus()
        spotify_bus = session_bus.get_object("org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2")

        # Interface for Spotify
        spotify_properties = dbus.Interface(spotify_bus, "org.freedesktop.DBus.Properties")
        
        # Get playback status
        playback_status = spotify_properties.Get("org.mpris.MediaPlayer2.Player", "PlaybackStatus")

        # Check if music is playing
        if playback_status == "Playing":
            metadata = spotify_properties.Get("org.mpris.MediaPlayer2.Player", "Metadata")

            # Extract relevant information
            title = metadata.get("xesam:title", "?")
            artist = metadata.get("xesam:artist", ["?"])[0]

            return f"{title}:{artist}"
        else:
            return None

    except Exception as e:
        return None

scrl_txt_obj = ScrollingText(limit=16, speed=0.5)
scrl_txt_obj.start_scrolling()
scrl_txt_obj.update_text("This is example scroll text")




# Constants
points = 20

# Serial initialization
ser = serial.Serial(port='/dev/ttyUSB0', baudrate=2000000)

display = Xlib.display.Display()

# Application alias dictionary
app_alias = {
    'plasmashell': 'Desktop',
    'Thorium-browser': 'Thorium',
    'kscreenlocker': 'Lockscreen',
    'ksmserver-logout-greeter': 'Power-menu',
}

# Get active window
def get_active_window():
    try:
        window = display.get_input_focus().focus
        wmclass = window.get_wm_class()
        if wmclass is None:
            window = window.query_tree().parent
            wmclass = window.get_wm_class()

        return wmclass[1] if wmclass and len(wmclass) > 1 else wmclass
    except Exception as error:
        print("An error occurred:", error)
        return 'Err'

# Application name parsing with alias
oldapp = ''
def parse_app(app):
    global oldapp

    if app:
        app = app_alias.get(app, app)
        app = (get_currently_playing() or app)



        if len(app) > 16:

            if app != oldapp:
                oldapp = app
                scrl_txt_obj.update_text(app)
                app = scrl_txt_obj.out
            else: 
                app = scrl_txt_obj.out


        else :
            pass #app.ljust(16, " ")

        return app
    return ""

# Change brightness
def change_brightness():
    val_old = 0
    while True:
        if avg_que:
            val = int(np.mean(avg_que))
            val = int(20+((val/100)*80))                     #change display brightness b/w 30 to 70
            if val_old != val:
                #print(val)
                os.system(f"ddcutil --sleep-multiplier .05 --noverify --bus 1 setvcp 10 {val}")
                val_old = val
            else:
                time.sleep(0.2)

avg_que = queue.deque(maxlen=points)

t1 = threading.Thread(target=change_brightness, daemon=True)
t1.start()  

# Read brightness
def read_brightness(timeout):
    try:
        line = ser.readline(timeout)
        if line:
            string = line.decode()
            num = int(string)
            avg_que.append(num)
    except Exception as error:
        print(error)

# Initial CPU usage measurement
psutil.cpu_percent(interval=None)

cpu_avg = queue.deque(maxlen=100)

def write_data(cls=False):
    while True:
        time1 = datetime.datetime.now().strftime("%I:%M")
        min_per = (int(time.strftime("%S")) % 15)
        min_per = int((min_per / 14) * 90)

        cpu_usage_raw = psutil.cpu_percent(interval=None)
        cpu_avg.append(cpu_usage_raw)
        cpu_usage = int(np.mean(cpu_avg))

        ram_usage = psutil.virtual_memory()[2]

        if not cls:
            app = parse_app(get_active_window())
        else:
            app = '                '

        try:
            data = f'<{time1},{app},{min_per},{cpu_usage},{ram_usage}>'
            ser.write(data.encode())
            read_brightness(timeout=None)
        except Exception as error:
            print("An error occurred:", error)

        if cls == True:
            break

# Start the data writing thread
t2 = threading.Thread(target=write_data, daemon=True)
t2.start()

# Keep the main thread running
t2.join()
t1.join()

#prgram exit
scrl_txt_obj.stop_scrolling()
