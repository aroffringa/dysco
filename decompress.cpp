#include "dyscostman.h"
#include "stmanmodifier.h"

using namespace dyscostman;

int main(int argc, char *argv[])
{
	register_dyscostman();
	
	if(argc < 2)
	{
		std::cerr << "Usage: decompress <ms>\n";
		return 0;
	}
	std::unique_ptr<casacore::MeasurementSet> ms(new casacore::MeasurementSet(argv[1], casacore::Table::Update));
	StManModifier modifier(*ms);
	
	bool isDataReplaced = modifier.InitColumnWithDefaultStMan("DATA", false);
	bool isWeightReplaced = modifier.InitColumnWithDefaultStMan("WEIGHT_SPECTRUM", true);
	
	if(isDataReplaced)
		modifier.MoveColumnData<casacore::Complex>("DATA");
	if(isWeightReplaced)
		modifier.MoveColumnData<float>("WEIGHT_SPECTRUM");

	if(isDataReplaced || isWeightReplaced)
	{
		StManModifier::Reorder(ms, argv[1]);
	}
	
	std::cout << "Finished.\n";
}
