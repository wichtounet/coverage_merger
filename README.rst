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
