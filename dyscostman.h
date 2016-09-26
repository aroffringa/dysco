#ifndef DYNSTACOM_STORAGE_MANAGER_H
#define DYNSTACOM_STORAGE_MANAGER_H

#include <casacore/tables/DataMan/DataManager.h>

#include <casacore/casa/Containers/Record.h>

#include <fstream>
#include <vector>

#include <stdint.h>

#include "uvector.h"
#include "dyscodistribution.h"
#include "dysconormalization.h"
#include "thread.h"

/**
* @file
* Contains DyscoStMan and its global register function
* register_dyscostman().
* 
* @defgroup Globals Global functions
* Contains the register_dyscostman() function.
*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern "C" {
#endif
	void register_dyscostman();
#ifndef DOXYGEN_SHOULD_SKIP_THIS
}
#endif

/**
 * @author André Offringa
 */
namespace dyscostman {

class DyscoStManColumn;

class DyscoStMan : public casacore::DataManager
{
public:
	explicit DyscoStMan(unsigned bitsPerFloat, unsigned bitsPerWeight, const casacore::String& name = "DyscoStMan");
	
	void SetGaussianDistribution(bool fitToMaximum)
	{
		_distribution = GaussianDistribution;
		_fitToMaximum = fitToMaximum;
	}
	
	void SetUniformDistribution()
	{
		_distribution = UniformDistribution;
		_fitToMaximum = true;
	}
	
	void SetStudentsTDistribution(bool fitToMaximum, double nu)
	{
		_distribution = StudentsTDistribution;
		_fitToMaximum = fitToMaximum;
		_studentTNu = nu;
	}

	void SetTruncatedGaussianDistribution(bool fitToMaximum, double truncationSigma)
	{
		_distribution = TruncatedGaussianDistribution;
		_fitToMaximum = fitToMaximum;
		_distributionTruncation = truncationSigma;
	}
	
	void SetNormalization(DyscoNormalization normalization)
	{
		_normalization = normalization;
	}

	/**
	* This constructor is called by Casa when it needs to create a DyscoStMan.
	* Casa will call makeObject() that will call this constructor.
	* When it loads an DyscoStMan for an existing MS, the "spec" parameter
	* will be empty, thus the class should initialize its properties
	* by reading them from the file.
	* The @p spec is used to make a new storage manager with specs similar to
	* another one.
	* @param name Name of this storage manager.
	* @param spec Specs to initialize this class with.
	*/
	DyscoStMan(const casacore::String& name, const casacore::Record& spec);

	/**
	* Copy constructor that initializes a storage manager with similar specs.
	* The columns are not copied: the new manager will be empty.
	*/
	DyscoStMan(const DyscoStMan& source);
	
	/** Destructor. */
	~DyscoStMan();

	DyscoStMan &operator=(const DyscoStMan& source) = delete;
	
	/** Polymorphical copy constructor, equal to DyscoStMan(const DyscoStMan&).
	* @returns Empty manager with specs as the source.
	*/
	virtual casacore::DataManager* clone() const final override { return new DyscoStMan(*this); }

	/** Type of manager
	* @returns "DyscoStMan". */
	virtual casacore::String dataManagerType() const final override { return "DyscoStMan"; }

	/** Returns the name of this manager as specified during construction. */
	virtual casacore::String dataManagerName() const final override { return _name; }

	/** Get manager specifications. Includes method settings, etc. Can be used
	* to make a second storage manager with 
	* @returns Record containing data manager specifications.
	*/
	virtual casacore::Record dataManagerSpec() const final override;

	/**
	* Get the number of rows in the measurement set.
	* @returns Number of rows in the measurement set.
	*/
	uint getNRow() const { return _nRow; }

	/**
	* Whether rows can be added.
	* @returns @c true
	*/
	virtual casacore::Bool canAddRow() const final override { return true; }

	/**
	* Whether rows can be removed.
	* @returns @c true (but only rows at the end can actually be removed)
	*/
	virtual casacore::Bool canRemoveRow() const final override { return true; }

	/**
	* Whether columns can be added.
	* @returns @c true (but restrictions apply; columns can only be added as long
	* as no writes have been performed on the set).
	*/
	virtual casacore::Bool canAddColumn() const final override { return true; }

	/**
	* Whether columns can be removed.
	* @return @c true (but restrictions apply -- still to be checked)
	* @todo Describe restrictons
	*/
	virtual casacore::Bool canRemoveColumn() const final override { return true; }

	/**
	* Create an object with given name and spec.
	* This methods gets registered in the DataManager "constructor" map.
	* The caller has to delete the object. New class will be
	* initialized via @ref DyscoStMan(const casacore::String& name, const casacore::Record& spec).
	* @returns An DyscoStMan with given specs.
	*/
	static casacore::DataManager* makeObject(const casacore::String& name,
																				const casacore::Record& spec)
	{
		return new DyscoStMan(name, spec);
	}

	/**
	* This function makes the OffringaStMan known to casacore. The function
	* is necessary for loading the storage manager from a shared library. It
	* should have this specific name ("register_" + storage manager's name in
	* lowercase) to be able to be automatically called when the library is
	* loaded. That function will forward the
	* call here.
	*/
	static void registerClass();
	
	/** TODO */
	void setDataColumnProperties(const std::string& columnName, size_t bitCount);
	
protected:
	/**
	* The number of rows that are actually stored in the file.
	*/
	uint64_t nBlocksInFile() const { return _nBlocksInFile; }
	
	size_t nRowsInBlock() const { return _rowsPerBlock; }
	
	size_t nAntennae() const { return _antennaCount; }
	
	size_t getBlockIndex(uint64_t row) { return row / _rowsPerBlock; }
	
	size_t getRowWithinBlock(uint64_t row) { return row % _rowsPerBlock; }
	
	uint64_t getRowIndex(size_t block) const { return uint64_t(block) * uint64_t(_rowsPerBlock); }
	
	bool areOffsetsInitialized() const { return _rowsPerBlock!=0; }
	
	void initializeRowsPerBlock(size_t rowsPerBlock, size_t antennaCount);
	
private:
	friend class DyscoStManColumn;
	
	const static unsigned short VERSION_MAJOR, VERSION_MINOR;
	
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	struct Header
	{
		/** Size of the total header, including column subheaders */
		uint32_t headerSize;
		/** Start offset of the column headers */
		uint32_t columnHeaderOffset;
		/** Number of columns and column headers */
		uint32_t columnCount;
		
		uint32_t rowsPerBlock;
		uint32_t antennaCount;
		uint32_t blockSize;
		
		/** File version number */
		uint16_t versionMajor, versionMinor;
		
		uint8_t dataBitCount;
		uint8_t weightBitCount;
		bool fitToMaximum;
		uint8_t distribution;
		uint8_t normalization;
		double studentTNu, distributionTruncation;
		
		// the column headers start here (first generic header, then column specific header)
	};
	
	struct GenericColumnHeader
	{
		/** size of generic header + column specific header */
		uint32_t columnHeaderSize;
	};
	
#endif
	
	void readCompressedData(size_t blockIndex, const DyscoStManColumn *column, unsigned char *dest, size_t size);
	
	void writeCompressedData(size_t blockIndex, const DyscoStManColumn *column, const unsigned char *data, size_t size);
	
	void readHeader();
	
	void writeHeader();

	void makeEmpty();
	
	void setFromSpec(const casacore::Record& spec);
	
	size_t getFileOffset(size_t blockIndex) const { return _blockSize * blockIndex + _headerSize; }
	
	// Flush and optionally fsync the data.
	// The AipsIO stream represents the main table file and can be
	// used by virtual column engines to store SMALL amounts of data.
	virtual casacore::Bool flush(casacore::AipsIO&, casacore::Bool doFsync) final override;
	
	// Let the storage manager create files as needed for a new table.
	// This allows a column with an indirect array to create its file.
	virtual void create(casacore::uInt nRow) final override;
	
	// Open the storage manager file for an existing table.
	// Return the number of rows in the data file.
	virtual void open(casacore::uInt nRow, casacore::AipsIO&) final override;
	
	// Create a column in the storage manager on behalf of a table column.
	// The caller will NOT delete the newly created object.
	// Create a scalar column.
	virtual casacore::DataManagerColumn* makeScalarColumn(const casacore::String& name,
		int dataType, const casacore::String& dataTypeID) final override;

	// Create a direct array column.
	virtual casacore::DataManagerColumn* makeDirArrColumn(const casacore::String& name,
		int dataType, const casacore::String& dataTypeID) final override;

	// Create an indirect array column.
	virtual casacore::DataManagerColumn* makeIndArrColumn(const casacore::String& name,
		int dataType, const casacore::String& dataTypeID) final override;

	virtual void resync(casacore::uInt nRow) final override;

	virtual void deleteManager() final override;

	// Prepare the columns, let the data manager initialize itself further.
	// Prepare is called after create/open has been called for all
	// columns. In this way one can be sure that referenced columns
	// are read back and partly initialized.
	virtual void prepare() final override;
	
	// Reopen the storage manager files for read/write.
	virtual void reopenRW() final override;
	
	// Add rows to the storage manager.
	virtual void addRow(casacore::uInt nrrow) final override;

	// Delete a row from all columns.
	virtual void removeRow(casacore::uInt rowNr) final override;

	// Do the final addition of a column.
	virtual void addColumn(casacore::DataManagerColumn*) final override;

	// Remove a column from the data file.
	virtual void removeColumn(casacore::DataManagerColumn*) final override;
	
	uint64_t _nRow;
	uint64_t _nBlocksInFile;
	uint32_t _rowsPerBlock;
	uint32_t _antennaCount;
	uint32_t _blockSize;
	
	unsigned _headerSize;
	altthread::mutex _mutex;
	std::unique_ptr<std::fstream> _fStream;
	
	std::string _name;
	unsigned _dataBitCount;
	unsigned _weightBitCount;
	bool _fitToMaximum;
	DyscoDistribution _distribution;
	DyscoNormalization _normalization;
	double _studentTNu, _distributionTruncation;

	std::vector<DyscoStManColumn*> _columns;
};

} // end of namespace

#endif
