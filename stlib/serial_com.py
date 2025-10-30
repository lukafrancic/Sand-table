import serial
import time
from queue import Queue
from enum import Enum, auto
import threading
from typing import TypedDict


class MsgType(Enum):
    none = b"\x60"
    headerA = b"\x61" # a
    headerB = b"\x62" # b
    position = b"\x63" # c
    speed = b"\x64" # d
    start = b"\x65" # e
    stop = b"\x66" # f
    clear = b"\x67" # g
    home = b"\x68" # h
    confirmRec = b"\x69" # i
    failedRec = b"\x70" #j
    getRBuffSize = b"\x71"
    sendRBuffSize = b"\x72"
    bufferFull = b"\x73"


class SerialStates(Enum):
    read_header = auto()
    read_msg = auto()
    read_data = auto()


class SendPacket(TypedDict):
    msg: bytes
    msg_arr: bytes


class SerialCOM:
    BAUDRATE = 115200
    LOOP_SLEEP_TIME = 0.05 # s
    HEADER = MsgType.headerA.value + MsgType.headerB.value
    BUFF_FULL_TIMEOUT = 1

    def __init__(self, COM: str):
        self._serial = serial.Serial(COM, baudrate=self.BAUDRATE, timeout=2)
        self._serial.flush()
        # wait a bit to establish COM
        time.sleep(1)

        self._pos_queue: Queue[bytes] = Queue(25)
        self._msg_queue: Queue[SendPacket] = Queue(25)

        self._ser_state = SerialStates.read_header
        self._last_msg = MsgType.confirmRec.value
        self._header_buff = [0, 0]
        self._is_running = False
        self._active_pos = False
        self._cur_pos = None
        self._last_pos_time = time.monotonic()


    def _add_item(self, msg: bytes):
        packet = SendPacket(msg=msg[2], msg_arr=msg)

        self._msg_queue.put(packet)
        

    def send_pos(self, pos: list[int, int]) -> None:
        """
        Adds the new position to a queue. The main loop handles the msg
        transaction.

        :param pos: list of [r, phi] as position in steps. R and Phi must be
            as int32!

        """
        if len(pos) != 2:
            raise ValueError("Invalid position")
        # if not (isinstance(pos[0], (int, np.int32)) and 
        # isinstance(pos[1], (int, np.int32))):
        #     raise ValueError("Values must be ints")
        r = int(pos[0])
        phi = int(pos[1])

        pos_r = r.to_bytes(4, "big", signed=True)
        pos_phi = phi.to_bytes(4, "big", signed=True)

        msg = self.HEADER + MsgType.position.value + pos_r + pos_phi

        print(f"Added to queue {msg}")
        self._pos_queue.put(msg, block=True, timeout=20)


    def update_speed(self, speed: int) -> None:
        """
        Update the speed value on the sand table. Adds the msg to the msg 
        queue.

        :param speed: speed is defined as steps per second as a uint16
        """

        if not isinstance(speed, int):
            raise TypeError(f"Speed must be an int not {type(speed)}!")
        
        if speed < 0 and speed > 2**16:
            raise ValueError("Speed must be a uint16!")
        
        val = speed.to_bytes(2, "big", signed=False)

        msg = self.HEADER + MsgType.speed.value + val

        self._add_item(msg)


    def home(self) -> None:
        """
        Send a command to home the sand table.
        """
        msg = self.HEADER + MsgType.home.value
        self._add_item(msg)

    
    def is_homed(self) -> bool:
        """
        Is the sand table homed?
        """
        #TODO
        pass


    def stop(self, clear: bool = False) -> None:
        """
        Stop the sand table.

        :param clear: whether to clear the position queue.
        """
        msg = self.HEADER + MsgType.stop.value
        self._add_item(msg)

        if clear:
            msg = self.HEADER + MsgType.clear.value
            self._add_item(msg)

    
    def start(self) -> None:
        """
        Start the sand table.
        """
        msg = self.HEADER + MsgType.start.value
        self._add_item(msg)


    def _loop(self) -> None:
        """
        We need a start off sequence -> is homed, stopped, buffer status

        state machine to keep track?

        let's just first see if this shit even works as expected.

        fokus... rabis posiljat msge in jih sproti tudi pobirati
        """
        
        if self._serial.in_waiting > 0:
            rec = self._serial.read_all()
            print("Ignoring all available msgs at startup:")
            print(f"\t{rec}\n")

        print("Starting the loop")
        while self._event.is_set():
            self._serial_send_postion()
            self._serial_send_msg()

            time.sleep(self.LOOP_SLEEP_TIME)


    def _serial_send_postion(self):
        if self._pos_queue.empty() and not self._active_pos:
            return
    
        if self._active_pos:
            t_ = time.monotonic()
            if (t_ - self._last_pos_time) < self.BUFF_FULL_TIMEOUT:
                return
        else:
            self._cur_pos = self._pos_queue.get()

        print(f"Sending msg: {self._cur_pos}")
        self._serial.write(self._cur_pos)

        ret = self._serial.read_until(self.HEADER, size=2)
        if not ret:
            print("Response pos timed out!")
            return

        msg = self._serial.read(1)
        if not msg:
            print("Response pos timed out!")
            return
        
        match msg:
            case MsgType.confirmRec.value:
                self._active_pos = False
                self._pos_queue.task_done()
                print(f"Msg confirmed {msg}")
            case MsgType.failedRec.value:
                print("Pos was denied")
                self._active_pos = True
            case MsgType.bufferFull.value:
                print("Buffer is full -> resend msg")
                self._active_pos = True
            case _:
                print(f"Received unexpected return pos msg {msg}")
                #TODO kaj res naredit v tem primeru?
                self._pos_queue.task_done()

        self._last_pos_time = time.monotonic()


    def _serial_send_msg(self):
        if self._msg_queue.empty():
            return
        
        item = self._msg_queue.get()
        print(f"Sending msg: {item['msg_arr']}")
        self._serial.write(item["msg_arr"])

        ret = self._serial.read_until(self.HEADER, size=2)
        if not ret:
            print("Response msg time out")
            return
        msg = self._serial.read(1)
        if not msg:
            print("Reponse msg timed out!")

        match msg:
            case MsgType.confirmRec.value:
                print(f"Msg confirmed {msg}")
                self._msg_queue.task_done()
                return
            case MsgType.failedRec.value:
                print("Msg was denied")
                #TODO send a retry msg?
                self._msg_queue.task_done()
                return
            case MsgType.sendRBuffSize.value:
                ret = self._serial.read(1)
                if not ret:
                    print("Failed to receive buff size")
                self._buffsize = int.from_bytes(ret)
                self._msg_queue.task_done()
                return
            case _:
                print(f"Received unexpected return msg {msg}")
                self._msg_queue.task_done()


    def begin_com(self):
        if self._is_running:
            print("The loop is already started")
            return
        
        self._thread = threading.Thread(target=self._loop)
        self._event = threading.Event()
        self._event.set()
        self._thread.start()
        self._is_running = True

    
    def stop_com(self):
        if not self._is_running:
            print("The loop is not active")
            return
        
        self._event.clear()
        self._thread.join()
        self._is_running = False
