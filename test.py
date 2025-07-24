import stlib as st

ww = st.Worker()

pt = st.SpiralAboutCenter(0, 150, num_revolutions=20)
pt.create()

ww.start_worker()

msg = {"msg":"t", "val":0}
ww.add_msg(msg)
ww.add_PathMaker(pt)

ww.end_workers()