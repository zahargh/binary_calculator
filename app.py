import serial
import tkinter as tk
import time
from tkinter import font, PhotoImage

arduino = serial.serial_for_url('rfc2217://localhost:4000', baudrate=9600)
time.sleep(2)

def send_button_press(value):
    arduino.write(value.encode())

def create_calculator_gui():
    window = tk.Tk()
    window.title("Binary Calculator")
    window.geometry("400x500")
    window.configure(bg="#1e1e2f")
    icon_image = PhotoImage(file="calculator.png")
    window.iconphoto(True, icon_image)

    for i in range(5):
        window.grid_rowconfigure(i, weight=1)
    for i in range(3):
        window.grid_columnconfigure(i, weight=1)

    def style_button(btn_text):
        return tk.Button(
            window,
            text=btn_text,
            width=8,
            height=2,
            bg="#3b3b4d",
            fg="white",
            font=font.Font(family="Arial", size=14, weight="bold"),
            relief="flat",
            bd=0,
            highlightbackground="#646464",
            highlightthickness=1,
            activebackground="#D5D5F5",
            activeforeground="#000000",
        )
    buttons = [
        ('1', '1'), ('0', '0'), ('+', '+'),
        ('-', '-'),('*', '*'), ('/', '/'), 
        ('(', '('), (')', ')'), ('=', '='),
        ('Delete', 'B'), ('Clear', 'C')
    ]

    for idx, (text, value) in enumerate(buttons):
        row, col = divmod(idx, 3)
        button = style_button(text)
        button.config(command=lambda v=value: send_button_press(str(v)))
        button.grid(row=row + 1, column=col, padx=10, pady=10, sticky="nsew")

    lcd_text = tk.StringVar()
    lcd_text.set("Ready!")    
    lcd_monitor = tk.Label(
        window,
        textvariable=lcd_text,
        font=font.Font(family="Consolas", size=24, weight="bold"),
        bg="#2c2c3c",
        fg="#D5D5F5",
        relief="ridge",
        bd=8,
        anchor="w",
        width=20
    )
    lcd_monitor.grid(row=0, column=0, columnspan=3, pady=20, padx=20, sticky="nsew")

    def update_lcd_monitor():
        if arduino.in_waiting > 0:
            lcd_output = arduino.readline().decode('utf-8').strip()
            lcd_text.set(lcd_output[-19:])
        window.after(100, update_lcd_monitor)  
    window.after(100, update_lcd_monitor)
    window.mainloop()

create_calculator_gui()