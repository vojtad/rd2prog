#ifndef ISPFRAME_H
#define ISPFRAME_H

#include <QByteArray>

/*
 * ISP Frame (from AT89c51RD2 datasheet)
 *
 * | Record Mark | Reclen | Load Offset | Record Type |   Data/Info     | Checksum |
 * |     ':'     | 1 byte |   2 byte    |   1 byte    | variable length |  1 byte  |
 *
 * Record Mark:
 *     Record Mark is the start of frame. This field must contain ?:?.
 *
 * Reclen:
 *     Reclen specifies the number of bytes of information or data which follows the Record Type field of the record.
 *
 * Load Offset:
 *     Load Offset specifies the 16-bit starting load offset of the data bytes, therefore this field is used
 *     only for Data Program Record (see Section ?ISP Commands Summary?).
 *
 * Record Type:
 *     Record Type specifies the command type. This field is used to interpret the remaining information
 *     within the frame. The encoding for all the current record types is described in Section ?ISP Commands Summary?.
 *
 * Data/Info:
 *     Data/Info is a variable length field. It consists of zero or more bytes encoded as pairs of hexadecimal
 *     digits. The meaning of data depends on the Record Type.
 *
 * Checksum:
 *     The two's complement of the 8-bit bytes that result from converting each pair of ASCII hexadecimal digits
 *     to one byte of binary, and including the Reclen field to and including the last byte of the Data/Info field.
 *     Therefore, the sum of all the ASCII pairs in a record after converting to binary, from the Reclen
 *     field to and including the Checksum field, is zero.
 */

namespace ISP
{
	enum Command // Record Type
	{
		ProgramCode			= 0x00, /* Bootloader will accept up to 128 (80h) data bytes.
									 * The data bytes should be 128 byte page flash boundary.
									 */

		WriteFunction		= 0x03, /*  data[0] | data[1] | Description
									 * ---------------------------------
									 *    01h   |   00h   | Erase block 0 (0000h - 1FFFh), only AT89C51RD2/ED2 (new)
									 *          |   20h   | Erase block 1 (2000h - 3FFFh), only AT89C51RD2/ED2 (new)
									 *          |   40h   | Erase block 2 (4000h - 7FFFh), only AT89C51RD2/ED2 (new)
									 *          |   80h   | Erase block 3 (8000h - BFFFh), only AT89C51RD2/ED2 (new)
									 *          |   C0h   | Erase block 4 (C000h - FFFFh), only AT89C51RD2/ED2 (new)
									 *    03h   |   00h   | Hardware Reset, only AT89C51RD2/ED2 (new)
									 *    04h   |   00h   | Erase SBV and BSB
									 *    05h   |   00h   | Program SSB Level 0
									 *          |   01h   | Program SSB Level 1
									 *    06h   |   00h   | Program BSB (value to write in data[2])
									 *          |   01h   | Program SBV (value to write in data[2])
									 *    07h   |    -    | Full Chip Erase (This command needs about 6 sec to be executed)
									 *    0Ah   |   04h   | Program BLJB fuse (value to write in data[2])
									 *          |   08h   | Program X2 fuse (value to write in data[2])
									 */

		DisplayFunction		= 0x04, /* data[0:1] = start address
									 * data[2:3] = end address
									 *
									 *  data[4] | Description
									 * -----------------------
									 *    00h   | Display Code
									 *    01h   | Blank Check
									 *    02h   | Display EEPROM data
									 */

		ReadFunction		= 0x05, /*  data[0] | data[1] | Description
									 * ---------------------------------
									 *    00h   |   00h   | Manufacturer Id
									 *          |   01h   | Device Id #1
									 *          |   02h   | Device Id #2
									 *          |   03h   | Device Id #3
									 *    07h   |   00h   | Read SSB
									 *          |   01h   | Read BSB
									 *          |   02h   | Read SBV
									 *          |   03h   | Read Hardware Byte, only T89C51RD2 (old)
									 *          |   06h   | Read Extra Byte, only AT89C51RD2/ED2 (new)
									 *    08h   |   00h   | Read Bootloader Version, only T89C51RD2 (old)
									 *    0Bh   |   00h   | Read Hardware Byte, only AT89C51RD2/ED2 (new)
									 *    0Eh   |   00h   | Read Device Boot ID1
									 *          |   01h   | Read Device Boot ID2
									 *    0Fh   |   00h   | Read Bootloader Version, only AT89C51RD2/ED2 (new)
									 */

		ProgramEEpromData	= 0x07, /* Program Nn EEprom Data Byte.
									 * Bootloader will accept up to 128 (80h) data bytes.
									 */
	};

	namespace WriteFunctionArgument
	{
		enum DataBytes
		{
			EraseBlock0			= 0x0000, // Erase block 0 (0000h - 1FFFh), only AT89C51RD2/ED2 (new)
			EraseBlock1			= 0x0120, // Erase block 1 (2000h - 3FFFh), only AT89C51RD2/ED2 (new)
			EraseBlock2			= 0x0140, // Erase block 2 (4000h - 7FFFh), only AT89C51RD2/ED2 (new)
			EraseBlock3			= 0x0180, // Erase block 3 (8000h - BFFFh), only AT89C51RD2/ED2 (new)
			EraseBlock4			= 0x01C0, // Erase block 4 (C000h - FFFFh), only AT89C51RD2/ED2 (new)
			HardwareReset		= 0x0300, // Hardware Reset, only AT89C51RD2/ED2 (new)
			EraseSBVAndBSB		= 0x0400, // Erase SBV and BSB
			ProgramSSBLevel0	= 0x0500, // Program SSB Level 0
			ProgramSSBLevel1	= 0x0501, // Program SSB Level 1
			ProgramBSB			= 0x0600, // Program BSB (value to write in next byte)
			ProgramSBV			= 0x0601, // Program SBV (value to write in next byte)
			FullChipErase		= 0x07,   // Full Chip Erase (This command needs about 6 sec to be executed)
			ProgramBLJBFuse		= 0x0A04, // Program BLJB fuse (value to write in next byte)
			ProgramX2Fuse		= 0x0A08, // Program X2 fuse (value to write in next byte)
		};
	};

	namespace ReadFunctionArgument
	{
		enum DataBytes
		{
			ManufacturerId				= 0x0000, // Manufacturer Id
			DeviceId1					= 0x0001, // Device Id #1
			DeviceId2					= 0x0002, // Device Id #2
			DeviceId3					= 0x0003, // Device Id #3
			ReadSSB						= 0x0700, // Read SSB
			ReadBSB						= 0x0701, // Read BSB
			ReadSBV						= 0x0702, // Read SBV
			ReadHardwareByteOld			= 0x0703, // Read Hardware Byte, only T89C51RD2 (old)
			ReadExtraByte				= 0x0706, // Read Extra Byte, only AT89C51RD2/ED2 (new)
			ReadBootloaderVersionOld	= 0x0800, // Read Bootloader Version, only T89C51RD2 (old)
			ReadHardwareByte			= 0x0B00, // Read Hardware Byte, only AT89C51RD2/ED2 (new)
			ReadDeviceBootID1			= 0x0E00, // Read Device Boot ID1
			ReadDeviceBootID2			= 0x0E01, // Read Device Boot ID2
			ReadBootloaderVersion		= 0x0F00, // Read Bootloader Version, only AT89C51RD2/ED2 (new)
		};
	};

	class ISPFrame
	{
		public:
			ISPFrame(qint8 recordType, qint32 argument = -1);
			~ISPFrame();

			void setArgument(qint32 argument = -1);
			void appendDataByte(quint8 byte);

			QByteArray data();

		private:
			quint8 m_reclen;
			quint16 m_loadOffset;
			quint8 m_recordType;
			QByteArray m_data;
			quint8 m_checksum;

			qint32 m_argument;
	};
};

//ISPFrame & operator<<(ISPFrame & isp, quint8 byte);

#endif // ISPFRAME_H
