from queue import Queue
from .path_maker import PathMaker
from .serial_com import SerialCOM
import threading
import time


class Worker:
    """
    Worker class that handles the given PathMakers and Communication to the
    Sand table.
    """
    def __init__(self, COM: str):
        self.q_path: Queue[PathMaker] = Queue()
        self.com = SerialCOM(COM)
        self._event = threading.Event()
        self._thread_active = False


    def add_PathMaker(self, item: PathMaker):
        self.q_path.put(item)
        print("Added to queue")


    def home(self):
        self.com.home()


    def stop(self, clear: bool = False):
        #TODO also clear pos queue and remove pathmakers
        self.com.stop(clear)


    def start(self):
        self.com.start()    


    def _position_worker(self):
        while self._event.is_set():
            if self.q_path.empty():
                time.sleep(0.5)
                continue
            try:
                pm = self.q_path.get()
                print("Got PathMaker")

                for val in pm:
                    # wait until a slot gets freed
                    self.com.send_pos(val)
                
                self.q_path.task_done()
                print("Path fully added to pos queue")

            except Exception as e:
                print(f"Fail: {e}")
                self.q_path.task_done()


    def start_worker(self):
        if self._thread_active:
            print("Thread already active")
            return
        
        self._worker_thread = threading.Thread(target=self._position_worker)
        self._event.set()
        self._worker_thread.start()
        self._thread_active = True
        self.com.begin_com()
        print("Started workers")

    
    def end_workers(self):
        if not self._thread_active:
            print("Thread not active")
            return
        
        self._event.clear()
        self.com.stop_com()
        self._worker_thread.join()
        self._thread_active = False
