.. _stack_analyzer:

Stack analyzer
###################

The stack analyzer module enables all the Zephyr options required to track
the stack information.
The analysis is performed on demand when the application calls
:cpp:func:`stack_analyzer_run` or :cpp:func:`stack_analyzer_print`.

Configuration
*************
Configure this module using the following options.

* ``STACK_ANALYZER``: enable the module.
* ``STACK_ANALYZER_USE_PRINTK``: use printk for stack statistics.
* ``STACK_ANALYZER_USE_LOG``: use the logger for stack statistics.
* ``STACK_ANALYZER_AUTO``: run the stack analyzer automatically.
  You do not need to add any code to the application when using this option.
* ``STACK_ANALYZER_AUTO_INTERVAL``: the time for which the module sleeps
  between consecutive printing of stack analysis in automatic mode.
* ``STACK_ANALYZER_AUTO_STACK_SIZE``: the stack for stack analyzer
  automatic thread.
* ``THREAD_NAME``: enable this option in the kernel to print the name of the
  thread instead of its ID.

API documentation
*****************

.. doxygengroup:: stack_analyzer
   :project: Zephyr
