#include "Client/Game/EmbeddedData.hpp"

#include <cctype>

namespace embedded
{
std::string fallbackFont = "\
89504e470d0a1a0a0000000d494844520000005f00000035080000000002\
486815000000017342495408e60a5b99000000097048597300000e9c0000\
0e750188281b78000002964944415458c3d5986b72e3300c8371ff1be8b4\
5f7fe80552949366bb3bdb4e47756319864842842249525393d410fd4ad5\
48ebd7fe3b9f5c7ff3b5c6b363a44416ac3942e9fac64742117fa2093501\
34900877c35be0037c3ab2901ae35d357e188b55eb05ffc1a12144c57f70\
c0b995f83af8ab6322684894fc1fb3b0e12bfe006a48d0c4e61ff27be625\
544bcdffb391fa73b5bffbb3f9b70f98bbc2f2ff535bbd4e46a5b122bfb3\
c0d2c2bcded5f5f876a06ba0eba8c7b0e791899f62cb98b36706351df945\
cc3aec7717b75e93f88ae2dda5eb42d11b2de1b3790e15385bf61ad7ddba\
7e58fbc39c1f95c2647bc33fe353f15ff9ed9fb3f238a341c826a9066276\
38d6c28fe88ba7bbff4e5fdf1ddfd3a35c2f3bda5865125536abcb151172\
81d5d8d2571aab0af7cfbdd2d0ee9ef4fd7c753d8402abcc24568badc5f0\
891d9c351abeaffd813f813f05fe8cd8c6cfcf7e1bdfe393a3ad94b53abf\
6b3f4cf94d3b15457e7faa7ffd3a7dd9e85eb1f5fd6df52f1ef475ea228d\
cbe359fd479f79d597efc6a97256f7948e1e97bd902a04776eb157e2bdc3\
e65cbda2a63691edfc47b7d29a3399537be992bf5c7115fef2ba672f4bfd\
e5825fa8ccbad5998bf9c69ddfc521fad85faf2fa63ad49ec6a6f2137fb6\
05e663a68ab389cc6bddbc01d9abc3a53fea113ffb99daf9733d65c8bc68\
d8757def4dee77b3f5dd60fa2bc7f1f30bca9d9a38dfe784f8a477e1335f\
e1876853e3072dd7f8b7f828fbe1bd2ecf2f15bec7e7adf1384df39ff4af\
8b8e4e4d5d94489bdf7ee0bef1393ebc1fb7dbb9fbe21ffcc4f727f8cbde\
eeda7327c951b7c3fbe5d359d90518f10908589da0123fd4e4a58b750ebd\
fa9313b39dc7f1ed5b148e18f2aabf27278075f074fa9ef109a7b3a257c6\
f375de8ddf3873bda335247d012bb3a1b318c1a3d30000000049454e44ae\
426082";

static bool hexLUTInitialized = false;

static char hexLUT[256];

static void initHexLut()
{
	if (hexLUTInitialized)
	{
		return;
	}

	for (unsigned int i = 0; i < 256; ++i)
	{
		hexLUT[i] = -1;
	}

	for (unsigned int i = 0; i < 9; ++i)
	{
		hexLUT['0' + i] = i;
	}

	for (unsigned int i = 0; i < 6; ++i)
	{
		hexLUT['A' + i] = 10 + i;
		hexLUT['a' + i] = 10 + i;
	}

	hexLUTInitialized = true;
}

std::vector<char> hexDecode(const std::string & encodedString)
{
	initHexLut();

	std::vector<char> ret;

	bool high = true;

	for (unsigned int i = 0; i < encodedString.size(); ++i)
	{
		char cur = hexLUT[(unsigned char) encodedString[i]];
		if (cur == -1)
			continue;

		if (high)
		{
			ret.push_back(cur * 16);
			high = false;
		}
		else
		{
			ret.back() += cur;
			high = true;
		}
	}

	return ret;
}

}
