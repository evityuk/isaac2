/**
 ** Isaac Genome Alignment Software
 ** Copyright (c) 2010-2014 Illumina, Inc.
 ** All rights reserved.
 **
 ** This software is provided under the terms and conditions of the
 ** GNU GENERAL PUBLIC LICENSE Version 3
 **
 ** You should have received a copy of the GNU GENERAL PUBLIC LICENSE Version 3
 ** along with this program. If not, see
 ** <https://github.com/illumina/licenses/>.
 **
 ** \file Nucleotides.hh
 **
 ** General tools and definitions to manipulate nucleotides.
 **
 ** \author Come Raczy
 **/

#ifndef iSAAC_OLIGO_NUCLEOTIDES_HH
#define iSAAC_OLIGO_NUCLEOTIDES_HH

#include <vector>
#include <boost/array.hpp>
#include <boost/assign.hpp>

#include "../common/StaticVector.hh"
#include "common/Debug.hh"

namespace isaac
{
namespace oligo
{

// valid ones are 0(A) 1(C) 2(G) and 3(T).
// for data, invalidOligo represents N
// for reference, invalidOligo indicates any non-ACGT base value
static const unsigned int INVALID_OLIGO = 4;

// It used to be that n in sequence would not match N in reference. This has caused problems generating NM bam tag
// changed to fix SAAC-697
static const char SEQUENCE_OLIGO_N = 'N';
static const char REFERENCE_OLIGO_N = 'N';


typedef common::StaticVector<unsigned int, 256> Translator;

inline Translator getTranslator(const bool withN = false, const unsigned defaultValue = INVALID_OLIGO)
{   
    Translator translator;
    translator.resize(translator.capacity(), defaultValue);
    translator['a'] = 0;
    translator['A'] = 0;
    translator['c'] = 1;
    translator['C'] = 1;
    translator['g'] = 2;
    translator['G'] = 2;
    translator['t'] = 3;
    translator['T'] = 3;
    if (withN)
    {
        translator['n'] = INVALID_OLIGO;
        translator['N'] = INVALID_OLIGO;
    }
    return translator;
}

inline unsigned int getValue(const char base)
{   
    static const oligo::Translator translator = getTranslator();
    assert(0 < base);
    assert(base < static_cast<long>(translator.size()));
    return translator[base];
}

static const boost::array<char, 5> allBases = boost::assign::list_of('A')('C')('G')('T')('N');
inline char getBase(unsigned int base, const bool upperCase = true)
{
    base = std::min(base, static_cast<unsigned int>(allBases.size()));    
    if (upperCase)
    {
        return allBases[base];
    }
    else
    {
        return allBases[base] + ('a' - 'A');
    }
}

/**
 * \return upercase base. Note that this one will not return N for bcl 0!
 */
inline char getUppercaseBase(unsigned int base)
{
    return getBase(base, true);
}

inline bool isBclN(const char bclByte)
{
    return !(bclByte & 0xfc);
}

inline unsigned char getQuality(char bclByte)
{
    return static_cast<unsigned char>(bclByte) >> 2;
}

/**
 * \return uppercase base or N for bcl N
 */
inline char getReferenceBaseFromBcl(unsigned char bcl)
{
    return oligo::isBclN(bcl) ? REFERENCE_OLIGO_N : getUppercaseBase(bcl & 0x03);
}

/**
 * \return uppercase base or N for bcl N
 */
inline char getSequenceBaseFromBcl(unsigned char bcl)
{
    return oligo::isBclN(bcl) ? SEQUENCE_OLIGO_N : getUppercaseBase(bcl & 0x03);
}


// Take a packed (2 bit per base) kmer and output to a string buffer
template <typename OutItr>
void unpackKmer(const unsigned long kmer, const unsigned long kmerLength, OutItr itr)
{
   for(unsigned int i = 0; i < kmerLength*2; i+=2)
   {
      * itr ++ = getBase(((kmer >> i) & 0x3ul));
   }
}


static const boost::array<char, 5> allReverseBases = boost::assign::list_of('T')('G')('C')('A')('N');
inline char getReverseBase(unsigned int base, const bool upperCase = true)
{
//    return base > 3 ? getBase(base, upperCase) : getBase((~base) & 3, upperCase);
    base = std::min(base, static_cast<unsigned int>(allReverseBases.size()));
    if (upperCase)
    {
        return allReverseBases[base];
    }
    else
    {
        return allReverseBases[base] + ('a' - 'A');
    }
}

inline char reverseBase(const char base)
{
    switch (base)
    {
    case 'a': return 't';
    case 'A': return 'T';
    case 'c': return 'g';
    case 'C': return 'G';
    case 'g': return 'c';
    case 'G': return 'C';
    case 't': return 'a';
    case 'T': return 'A';
    case 'n': return 'n';
    default : return 'N';
    }
}

/**
 * \brief reverse-complements the base bits of a bcl byte
 *
 * \returns 0 for 0, reverse-complemented lower bits with quality bits unchanged
 */
inline unsigned char getReverseBcl(const unsigned char bcl)
{
    return !isBclN(bcl) ? (bcl & 0xfc) | (0x3 - (bcl & 0x03)) : 0;
}

template <int bitsPerBase, typename KmerT>
std::string bases(KmerT kmer, unsigned kmerLength)
{
    static const unsigned kmerMask = ~(~0U << bitsPerBase);

    std::string s;
    while (kmerLength--)
    {
        s.push_back(isaac::oligo::getBase((kmer >> bitsPerBase * kmerLength) & kmerMask));
    }
    return s;
}

template <unsigned bitsPerBase, typename KmerT>
struct Bases
{
    const KmerT kmer_;
    const unsigned kmerLength_;
    static const unsigned bitsPerBase_ = bitsPerBase;
    static const unsigned kmerMask_ = ~(~0U << bitsPerBase);
    Bases(const KmerT kmer, unsigned kmerLength) : kmer_(kmer), kmerLength_(kmerLength)
    {

    }
};

template <unsigned bitsPerBase, typename KmerT>
struct ReverseBases
{
    const KmerT kmer_;
    const unsigned kmerLength_;
    static const unsigned bitsPerBase_ = bitsPerBase;
    static const unsigned kmerMask_ = ~(~0U << bitsPerBase);
    ReverseBases(const KmerT kmer, unsigned kmerLength) : kmer_(kmer), kmerLength_(kmerLength)
    {

    }
};

template <typename BasesT>
std::ostream & printBases(std::ostream &os, const BasesT &b)
{
    unsigned pos = b.kmerLength_;
    while (pos--)
    {
        static const unsigned kmerMask_ = BasesT::kmerMask_;
        os << isaac::oligo::getBase((b.kmer_ >> BasesT::bitsPerBase_ * pos) & kmerMask_);
    }
    return os;
}

template <typename BasesT>
std::ostream & printReverseBases(std::ostream &os, const BasesT &b)
{
    for (unsigned pos = 0; b.kmerLength_ > pos; ++pos)
    {
        static const unsigned kmerMask_ = BasesT::kmerMask_;
        os << isaac::oligo::getReverseBase(unsigned((b.kmer_ >> BasesT::bitsPerBase_ * pos) & kmerMask_));
    }
    return os;
}

template <typename BasesIteratorT>
inline std::string bclToString(BasesIteratorT basesIterator, unsigned length)
{
    std::string ret;
    while(length--)
    {
        ret += oligo::isBclN(*basesIterator) ? SEQUENCE_OLIGO_N :
            oligo::getBase(0x3 & *basesIterator);
        ++basesIterator;
    }
    return ret;
}

inline std::string bclToRString(const unsigned char *basesIterator, unsigned length)
{
    std::string ret;
    while(length--)
    {
        ret += oligo::isBclN(*(basesIterator + length)) ? SEQUENCE_OLIGO_N :
            oligo::reverseBase(oligo::getBase(0x3 & *(basesIterator + length)));
    }
    return ret;
}

template <typename FwdIteratorT>
unsigned long pack32BclBases(FwdIteratorT bcl)
{
    unsigned long ret = 0;
    ret |= (0x3ul & *bcl++);
    ret |= (0x3ul & *bcl++) <<  2;
    ret |= (0x3ul & *bcl++) <<  4;
    ret |= (0x3ul & *bcl++) <<  6;
    ret |= (0x3ul & *bcl++) <<  8;
    ret |= (0x3ul & *bcl++) << 10;
    ret |= (0x3ul & *bcl++) << 12;
    ret |= (0x3ul & *bcl++) << 14;
    ret |= (0x3ul & *bcl++) << 16;
    ret |= (0x3ul & *bcl++) << 18;
    ret |= (0x3ul & *bcl++) << 20;
    ret |= (0x3ul & *bcl++) << 22;
    ret |= (0x3ul & *bcl++) << 24;
    ret |= (0x3ul & *bcl++) << 26;
    ret |= (0x3ul & *bcl++) << 28;
    ret |= (0x3ul & *bcl++) << 30;
    ret |= (0x3ul & *bcl++) << 32;
    ret |= (0x3ul & *bcl++) << 34;
    ret |= (0x3ul & *bcl++) << 36;
    ret |= (0x3ul & *bcl++) << 38;
    ret |= (0x3ul & *bcl++) << 40;
    ret |= (0x3ul & *bcl++) << 42;
    ret |= (0x3ul & *bcl++) << 44;
    ret |= (0x3ul & *bcl++) << 46;
    ret |= (0x3ul & *bcl++) << 48;
    ret |= (0x3ul & *bcl++) << 50;
    ret |= (0x3ul & *bcl++) << 52;
    ret |= (0x3ul & *bcl++) << 54;
    ret |= (0x3ul & *bcl++) << 56;
    ret |= (0x3ul & *bcl++) << 58;
    ret |= (0x3ul & *bcl++) << 60;
    ret |= (0x3ul & *bcl)   << 62;
    return ret;
}

template <typename FwdIteratorT>
unsigned long packBclBases(FwdIteratorT bclBegin, FwdIteratorT bclEnd)
{
    unsigned long ret = 0;
    const int length = bclEnd - bclBegin;
    ISAAC_ASSERT_MSG(length <= 32, "Cannot pack more than 64 bases");

    for(int i = 0; i < length; ++i)
    {
    	ret |= (0x3ul & *bclBegin++) << (i*2);
    }
    // Nothing left to do with remaining: 32 - length, 0 init will be fine
    return ret;
}

} // namespace oligo
} // namespace isaac

#endif // #ifndef iSAAC_OLIGO_NUCLEOTIDES_HH
