To create additional output plugins, place Python modules in this directory.

Plugins need to subclass shellguard.core.output.Output and define at least the
methods 'start', 'stop' and 'write'

    import shellguard.core.output

    class Output(shellguard.core.output.Output):

        def start(self):

        def stop(self):

        def write( self, event ):


