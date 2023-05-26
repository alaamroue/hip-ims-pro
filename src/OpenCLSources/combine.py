def combine_files(filenames, output_file):
    with open(output_file, 'w') as output:
        for filename in filenames:
            with open(filename, 'r') as file:
                output.write(file.read())

# Example usage
filenames = ['CLUniversalHeader.clh','CLDomainCartesian.clh','CLFriction.clh','CLSolverHLLC.clh','CLDynamicTimestep.clh','CLSchemePromaides.clh','CLBoundaries.clh','CLDomainCartesian.clc','CLFriction.clc','CLSolverHLLC.clc','CLDynamicTimestep.clc','CLSchemePromaides.clc','CLBoundaries.clc']

output_filename = 'combined_files.txt'

combine_files(filenames, output_filename)

print(f"Combined files: {filenames} into {output_filename}")
