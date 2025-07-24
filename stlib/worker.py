import asyncio
from queue import Queue
from .path_maker import PathMaker
from .serial_com import Communication, Messagge
import threading


# class Worker:
#     """
#     Worker class that handles the given PathMakers and Communication to the
#     Sand table.
#     """
#     def __init__(self):
#         self.q_path = asyncio.Queue()
#         self.q_com_pos = Queue(25)
#         self.q_com = Queue()

#         self.com = Communication("COM9", self.q_com, self.q_com_pos)


#     def add_PathMaker(self, item: PathMaker):
#         self.q_path.put(item)
#         print("Added to queue")


#     def add_msg(self, msg: Messagge):
#         """
#         Send a msg to the Sand table.

#         :param msg: Messagge is a TypedDict with params 
#         """
#         self.q_com.put(msg)
#         print("sent msg")
    

#     async def _update_position_worker(self):
#         while True:
#             try:
#                 pm = await self.q_path.get()
#                 print("Got PathMaker")

#                 for val in pm:
#                     # wait until a slot gets freed
#                     self.q_com_pos.put(val, block=True)
                
#                 self.q_path.task_done()
#                 print("finished task")

#             except Exception as e:
#                 print(f"Fail: {e}")
#                 self.q_path.task_done()


#     def start_worker(self):
#         loop = asyncio.get_event_loop()
#         if loop.is_running():
#             asyncio.create_task(self.com.loop)
#             asyncio.create_task(self._update_position_worker)
#         else:
#             loop.create_task(self.com.loop)
#             loop.create_task(self._update_position_worker)

#         print("Started workers")

    

class Worker:
    """
    Worker class that handles the given PathMakers and Communication to the
    Sand table.
    """
    def __init__(self):
        self.q_path = Queue()
        self.q_com_pos = Queue(25)
        self.q_com = Queue()

        self.com = Communication("COM9", self.q_com, self.q_com_pos)


    def add_PathMaker(self, item: PathMaker):
        self.q_path.put(item)
        print("Added to queue")


    def add_msg(self, msg: Messagge):
        """
        Send a msg to the Sand table.

        :param msg: Messagge is a TypedDict with params 
        """
        self.q_com.put(msg)
        print("sent msg")
    

    def _update_position_worker(self):
        while True:
            try:
                pm = self.q_path.get()
                print("Got PathMaker")

                for val in pm:
                    # wait until a slot gets freed
                    self.q_com_pos.put(val, block=True)
                
                self.q_path.task_done()
                print("finished task")

            except Exception as e:
                print(f"Fail: {e}")
                self.q_path.task_done()


    def start_worker(self):
        self._loop_thread = threading.Thread(target=self.com.loop)
        self._worker_thread = threading.Thread(target=self._update_position_worker)
        
        self._loop_thread.start()
        self._worker_thread.start()
        print("Started workers")

    
    def end_workers(self):
        self._loop_thread.join()
        self._worker_thread.join()
