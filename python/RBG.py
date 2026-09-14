import array, time
from machine import Pin
import rp2
from rp2 import PIO, StateMachine, asm_pio
import array, time
from machine import Pin
import rp2
from rp2 import PIO, StateMachine, asm_pio

# Configure the number of WS2812 LEDs to 1.
NUM_LEDS = 1

@asm_pio(sideset_init=PIO.OUT_LOW, out_shiftdir=PIO.SHIFT_LEFT, autopull=True, pull_thresh=24)
def ws2812():
    T1 = 2
    T2 = 5
    T3 = 3
    label("bitloop")
    out(x, 1)               .side(0)    [T3 - 1] 
    jmp(not_x, "do_zero")   .side(1)    [T1 - 1] 
    jmp("bitloop")          .side(1)    [T2 - 1] 
    label("do_zero")
    nop()                   .side(0)    [T2 - 1]

# Create the StateMachine with the ws2812 program, outputting on Pin(20).
sm = StateMachine(0, ws2812, freq=8000000, sideset_base=Pin(25))

# Start the StateMachine, it will wait for data on its FIFO.
sm.active(1)

# Display a pattern on the LEDs via an array of LED RGB values.
ar = array.array("I", [0 for _ in range(NUM_LEDS)])

# Function to create a color (R, G, B)
def create_color(r, g, b):
    return (r << 16) + (g << 8) + b

# Function to gradually change color
def change_color(from_color, to_color, steps):
    r1, g1, b1 = (from_color >> 16) & 0xFF, (from_color >> 8) & 0xFF, from_color & 0xFF
    r2, g2, b2 = (to_color >> 16) & 0xFF, (to_color >> 8) & 0xFF, to_color & 0xFF

    # Gradually transition the RGB values
    for step in range(steps):
        r = r1 + (r2 - r1) * step // steps
        g = g1 + (g2 - g1) * step // steps
        b = b1 + (b2 - b1) * step // steps
        color = create_color(r, g, b)
        for i in range(NUM_LEDS): 
            ar[i] = color
        sm.put(ar, 8)
        time.sleep_ms(10)  # Adjust speed of transition

# Define colors to cycle through (red, green, blue)
colors = [
    create_color(127, 0, 0),  # Red
    create_color(0, 127, 0),  # Green
    create_color(0, 0, 127),  # Blue
    create_color(127, 127, 0),  # Yellow
    create_color(0, 127, 127),  # Cyan
    create_color(127, 0, 127),  # Magenta
    create_color(127, 127, 127)  # White
]

# Main loop to cycle through colors
while True:
    for i in range(len(colors)):
        from_color = colors[i]
        to_color = colors[(i + 1) % len(colors)]  # Next color (wrap around)
        print(f"Transitioning from {from_color} to {to_color}")
        change_color(from_color, to_color, 100)  # Transition steps