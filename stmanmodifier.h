#ifndef STMAN_MODIFIER_H
#define STMAN_MODIFIER_H

#include "dyscostman.h"

#include <casacore/ms/MeasurementSets/MeasurementSet.h>

#include <casacore/tables/Tables/ArrColDesc.h>
#include <casacore/tables/Tables/ArrayColumn.h>

#include <iostream>

namespace dyscostman {

/**
 * Modify the storage manager of columns in a measurement set.
 * Used by the compress and decompress tools.
 * @author André Offringa
 */
class StManModifier
{
public:
	/**
	 * Constructor.
	 * @param ms Measurement set on which to operate.
	 */
	explicit StManModifier(casacore::MeasurementSet& ms) : _ms(ms)
	{	}
	
	/**
	 * Get the name of a storage manager for a given
	 * column and columntype. The column should be an arraycolumn.
	 * @tparam T The type of the column.
	 * @param columnName Name of the column.
	 */
	template<typename T>
	std::string GetStorageManager(const std::string& columnName)
	{
		casacore::ArrayColumn<T> dataCol(_ms, columnName);
		return dataCol.columnDesc().dataManagerType();
	}

	/**
	 * Rename the old column and create a new column with the default storage manager.
	 * After all columns have been renamed, the operation should be finished by a call
	 * to MoveColumnData() if @c true was returned.
	 * @param columnName Name of column.
	 * @param isWeight true if a weight (float) column should be created, or false to create
	 * a complex column.
	 * @returns @c true if the storage manager was changed.
	 */
	bool InitColumnWithDefaultStMan(const std::string& columnName, bool isWeight)
	{
		std::string dataManager;
		if(isWeight)
			dataManager = GetStorageManager<float>(columnName);
		else
			dataManager = GetStorageManager<casacore::Complex>(columnName);
		std::cout << "Current data manager of " + columnName + " column: " << dataManager << '\n';
		
		if(dataManager == "DyscoStMan")
		{
			std::string tempName = std::string("TEMP_") + columnName;
			std::cout << "Renaming old " + columnName + " column...\n";
			_ms.renameColumn(tempName, columnName);
			
			if(isWeight)
			{
				casacore::ArrayColumn<float> oldColumn(_ms, tempName);
				casacore::ArrayColumnDesc<float> columnDesc(columnName, oldColumn.shape(0));
				_ms.addColumn(columnDesc);
			} else {
				casacore::ArrayColumn<casacore::Complex> oldColumn(_ms, tempName);
				casacore::ArrayColumnDesc<casacore::Complex> columnDesc(columnName, oldColumn.shape(0));
				_ms.addColumn(columnDesc);
			}
			return true;
		} else {
			return false;
		}
	}

	/**
	 * Rename the old column and create a new column with the DyscoStMan.
	 * After all columns have been renamed, the operation should be finished by a call
	 * to MoveColumnData() if @c true was returned.
	 * @param columnName Name of column.
	 * @param bitsPerComplex If a DyscoStMan needs to be constructed, set this number
	 * of bits per data value.
	 * @param bitsPerWeight If a DyscoStMan needs to be constructed, set this number
	 * of bits per weight value.
	 * @returns @c true if the storage manager was changed.
	 */
	template<typename T>
	bool PrepareReplacingColumn(const std::string& columnName, const std::string& newDataManager, unsigned bitsPerComplex, unsigned bitsPerWeight, casacore::IPosition& shape)
	{
		const std::string dataManager = GetStorageManager<T>(columnName);
		std::cout << "Current data manager of " + columnName + " column: " << dataManager << '\n';
		
		if(dataManager != newDataManager)
		{
			std::string tempName = std::string("TEMP_") + columnName;
			std::cout << "Renaming old " + columnName + " column...\n";
			_ms.renameColumn(tempName, columnName);
			
			casacore::ArrayColumn<T> oldColumn(_ms, tempName);
			shape = oldColumn.shape(0);
			return true;
		} else {
			return false;
		}
	}

	/**
	 * Finish changing the storage manager for a column.
	 * This should be called for each changed column after all calls to the PrepareReplacing...()
	 * method have been performed, and the data managers have been added.
	 * @tparam T Type of array column
	 * @param columnName Name of column.
	 */
	template<typename T>
	void MoveColumnData(const std::string& columnName)
	{
		std::cout << "Copying values for " << columnName << " ...\n";
		std::string tempName = std::string("TEMP_") + columnName;
		std::unique_ptr<casacore::ArrayColumn<T> > oldColumn(new casacore::ArrayColumn<T>(_ms, tempName));
		casacore::ArrayColumn<T> newColumn(_ms, columnName);
		copyValues(newColumn, *oldColumn, _ms.nrow());
		oldColumn.reset();
		
		std::cout << "Removing old column...\n";
		_ms.removeColumn(tempName);
	}
	
	/**
	 * Perform a deep copy of the measurement set, and replace the old set
	 * with the new measurement set. This is useful to free unused space.
	 * When removing a column from a storage manager,
	 * the space occupied by the column is not freed. To make sure that any space
	 * occupied is freed, this copy is necessary.
	 * @param ms Pointer to old measurement set
	 * @param msLocation Path of the measurent set.
	 */
	static void Reorder(std::unique_ptr<casacore::MeasurementSet>& ms, const std::string& msLocation)
	{
		std::cout << "Reordering ms...\n";
		std::string tempName = msLocation;
		while(*tempName.rbegin() == '/') tempName.resize(tempName.size()-1);
		tempName += "temp";
		ms->deepCopy(tempName, casacore::Table::New, true);
		ms->markForDelete();

		// Destruct the old measurement set, and load new one
		ms.reset(new casacore::MeasurementSet(tempName, casacore::Table::Update));
		ms->rename(std::string(msLocation), casacore::Table::New);
	}
	
private:
	casacore::MeasurementSet &_ms;

	template<typename T>
	void copyValues(casacore::ArrayColumn<T>& newColumn, casacore::ArrayColumn<T>& oldColumn, size_t nrow)
	{
		casacore::Array<T> values;
		for(size_t row=0; row!=nrow; ++row)
		{
			oldColumn.get(row, values);
			newColumn.put(row, values);
		}
	}
};

}

#endif
