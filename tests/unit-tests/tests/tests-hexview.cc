#include <gtkmm.h>

#include "../test.hh"

#include <utils.hh>
#include <hexview.hh>

TESTSUITE(hexview)
{
	class MarkFixture
	{
	public:
		MarkFixture()
		{
		}

		bool getStartAndSize(const std::string &mask, uint64_t &startAddress, size_t &size)
		{
			unsigned zeros = 0;
			unsigned crosses = 0;

			// Find first cross
			for (unsigned i = 20; i < 20+49; i++) {
				if (mask[i] == '0')
					zeros++;
				else if (mask[i] == 'X')
					break;
			}

			// Count crosses
			for (unsigned i = 0; i < mask.size(); i++) {
				if (mask[i] == 'X')
					crosses++;
			}

			startAddress = zeros / 2;
			size = crosses / 2;

			printf("Start: %llu, size %zu\n", (unsigned long long)startAddress, size);

			return true;
		}

		bool verifyMask(const std::string &mask, HexView::LineOffsetList_t lst)
		{
			unsigned i = 0;
			unsigned startX = 0;
			unsigned xCount = 0;
			unsigned line = 0;

			while (1) {
				if (i == mask.size())
					break;

				if (startX != 0) {
					xCount++;
					if (mask[i] != 'X' && mask[i] != ' ') {
						printf("X ending at %u\n", i);
						ASSERT_TRUE(!lst.empty());

						HexView::LineOffset cur = lst.front();
						lst.pop_front();

						ASSERT_TRUE(cur.m_offset == startX);
						ASSERT_TRUE(cur.m_count == xCount);
						ASSERT_TRUE(cur.m_line == line);
					}
				} else if (mask[i] == 'X') {
					printf("X starting at %u\n", i);
					startX = i;
					xCount = 0;
				} else if (mask[i] == '\n')
					line++;

				i++;
			}

			return lst.empty();
		}
	};

	static uint8_t data[] =
	{
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
	};

	TEST(endianness)
	{
		// We assume x86 for the unit tests.
		ASSERT_TRUE(cpu_is_little_endian());
	}

	TEST(test8)
	{
		HexView h;

		std::string s;

		s = h.getLine8(data);
		ASSERT_TRUE(s == "00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff");
		s = h.getAscii(data);
		ASSERT_TRUE(s == "..\"3DUfw........");
	}

	TEST(test16)
	{
		HexView h;

		std::string s;

		s = h.getLine16((uint16_t *)data, false);
		ASSERT_TRUE(s == "0011 2233 4455 6677 8899 aabb ccdd eeff");

		s = h.getLine16((uint16_t *)data, true);
		ASSERT_TRUE(s == "1100 3322 5544 7766 9988 bbaa ddcc ffee");
	}

	TEST(test32)
	{
		HexView h;

		std::string s;

		s = h.getLine32((uint32_t *)data, false);
		ASSERT_TRUE(s == "00112233 44556677 8899aabb ccddeeff");

		s = h.getLine32((uint32_t *)data, true);
		ASSERT_TRUE(s == "33221100 77665544 bbaa9988 ffeeddcc");
	}

	TEST(test64)
	{
		HexView h;

		std::string s;

		s = h.getLine64((uint64_t *)data, true);
		ASSERT_TRUE(s == "7766554433221100 ffeeddccbbaa9988");

		s = h.getLine64((uint64_t *)data, false);
		ASSERT_TRUE(s == "0011223344556677 8899aabbccddeeff");
	}

	TEST(update)
	{
		HexView h;

		h.addData((void *)data, 0x1000, sizeof(data));

		std::string s8LE = h.handleAllData(8, true, true);
		std::string s16BE = h.handleAllData(16, false, false);
		std::string s32BE = h.handleAllData(32, false, false);
		std::string s64BE = h.handleAllData(64, false, false);

		ASSERT_TRUE(s8LE ==  "0x0000000000001000  00 11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff  ..\"3DUfw........\n");
		ASSERT_TRUE(h.m_addressToLineMap[0x1000] == 0ULL);

		ASSERT_TRUE(s16BE == "0x0000000000001000  0011 2233 4455 6677 8899 aabb ccdd eeff          ..\"3DUfw........\n");
		ASSERT_TRUE(s32BE == "0x0000000000001000  00112233 44556677 8899aabb ccddeeff              ..\"3DUfw........\n");
		ASSERT_TRUE(s64BE == "0x0000000000001000  0011223344556677 8899aabbccddeeff                ..\"3DUfw........\n");
	}

	TEST(mark, MarkFixture)
	{
		std::string m1 = "0x0000000000000000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  .................\n";
		std::string m2 = "0x0000000000000000  00 00 00 00 00 00 00 00 XX XX 00 00 00 00 00 00  ..........x......\n";
		std::string m3 = "0x0000000000000000  0000 0000 0000 0000 0000 0000 0000 00XX          ................x\n"
				         "0x0000000000000010  XXXX 0000 0000 0000 0000 0000 0000 0000          xx...............\n";
		bool res;

		uint64_t start;
		size_t size;
		HexView h;

		h.m_addressToLineMap[0] = 0;
		h.m_addressToLineMap[0x10] = 0x10;
		res = getStartAndSize(m1, start, size);
		ASSERT_TRUE(res);

		HexView::LineOffsetList_t lst = h.getMarkRegions(start, size, 8);
		res = verifyMask(m1, lst);
		ASSERT_TRUE(res);


		res = getStartAndSize(m2, start, size);
		ASSERT_TRUE(res);

		lst = h.getMarkRegions(start, size, 8);
		res = verifyMask(m2, lst);
		ASSERT_TRUE(res);


		res = getStartAndSize(m3, start, size);
		ASSERT_TRUE(res);

		lst = h.getMarkRegions(start, size, 16);
		res = verifyMask(m3, lst);
		ASSERT_TRUE(res);
	}
}
