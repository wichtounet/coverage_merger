coverage_merger
###############

Attempt to merge several coverage files in Cobertura XML format.

This project is not made to be perfect, neither general purpose, this project
is especially made to put the data to Sonar so that complex build can be tested
under several configurations and the profiles merged together. This gives that
advantage that Sonar recomputes the package and global coverage rates.
Therefore, it is easier to merge files.  Moreover, this project is made for
gcovr-generated files from gcov-collected profiles. It may not be compatible
with Java-generated Cobertura files.

Features:
 * Add source packages to the target xml so that they are all present
 * Ignore some packages
 * Add classes from different packages in the merged file to maximize coverage

Ideally, the tool should also try to maximize the coverage by merging classes
together. For now, nothing of that sort is done. The first step could be simply
be to take the class with maximum coverage and simply put in the target.

Build
#####

You can build and install the tool with the following commands after having clone the repository:

.. code:: bash
   make
   sudo make install

Usage
#####

You can execute the tool with a set of files and a target file, for instance:

.. code:: bash
    merger file1.xml file2.xml target.xml

You can also ignore some files with the --ignore=xxx switch (ignore everything
starting with xxx).
