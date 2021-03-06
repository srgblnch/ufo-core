import tifffile
from gi.repository import GLib, Ufo
from nose.tools import raises
from common import tempdir, disable


@raises(GLib.GError)
def test_wrong_connection():
    from ufo import Read, FlatFieldCorrect, Write

    darks = Read()
    flats = Read()
    radios = Read()
    write = Write()
    ffc = FlatFieldCorrect()

    g = Ufo.TaskGraph()
    g.connect_nodes_full(radios.task, ffc.task, 0)
    g.connect_nodes_full(darks.task, ffc.task, 1)
    g.connect_nodes_full(flats.task, ffc.task, 0)
    g.connect_nodes(ffc.task, write.task)

    sched = Ufo.Scheduler()
    sched.run(g)


def test_task_count():
    from ufo import DummyData, Write, Average

    with tempdir() as d:
        generate = DummyData(number=5, width=512, height=512)
        write = Write(filename=d.path('foo-%i.tif'))
        average = Average()

        write(average(generate())).run().join()

        assert(generate.task.props.num_processed == 0)
        assert(average.task.props.num_processed == 5)
        assert(write.task.props.num_processed == 1)


@disable
def test_broadcast():
    from ufo import Generate, Writer
    import glob

    with tempdir() as d:
        generate = Generate(number=5, width=512, height=512)
        write1 = Writer(filename=d.path('foo-%i.tif'))
        write2 = Writer(filename=d.path('bar-%i.tif'))

        g = Ufo.TaskGraph()
        g.connect_nodes(generate.task, write1.task)
        g.connect_nodes(generate.task, write2.task)

        sched = Ufo.Scheduler()
        sched.run(g)

        foos = glob.glob(d.path('foo-*'))
        bars = glob.glob(d.path('bar-*'))
        assert(len(foos) == 5)
        assert(len(bars) == 5)


def test_resource_info():
    resources = Ufo.Resources()
    nodes = resources.get_gpu_nodes()
    assert(nodes)
    node = nodes[0]
    assert(node.get_info(Ufo.GpuNodeInfo.LOCAL_MEM_SIZE) > 0)
    assert(node.get_info(Ufo.GpuNodeInfo.GLOBAL_MEM_SIZE) > node.get_info(Ufo.GpuNodeInfo.LOCAL_MEM_SIZE))
