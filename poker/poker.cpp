#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "curses.h"

#include "ThreadHelper.h"
#include "math.h"
#include "Chronometer.h"

#include <vector>
#include <algorithm>
#include <cassert>
#include <thread>
#include <mutex>
#include <deque>

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#define ABS(a) ( (a) > 0 ? (a) : -(a) )

enum class CardValue : uint8_t
{
	Deuce = 0,
	Three,
	Four,
	Five,
	Six,
	Seven,
	Eight,
	Nine,
	Ten,
	Jack,
	Queen,
	King,
	Ace,

	Count
};

enum class CardColor : uint8_t
{
	Spade,
	Heart,
	Diamond,
	Club,

	Count
};

enum class HandType : uint8_t
{
	HighCard,
	OnePair,
	TwoPair,
	ThreeOfAKind,
	Straight,
	Flush,
	FullHouse,
	FourOfAKind,
	StraightFlush
};

CardColor CharToCardColor(char c)
{
	switch (c)
	{
	case 's':
	case 'S': return CardColor::Spade;
	case 'h':
	case 'H': return CardColor::Heart;
	case 'd':
	case 'D': return CardColor::Diamond;
	case 'c':
	case 'C': return CardColor::Club;
	default:
		return static_cast<CardColor>(-1);
	}
}

CardValue CharToCardValue(char c)
{
	switch (c)
	{
	case '2': return CardValue::Deuce;
	case '3': return CardValue::Three;
	case '4': return CardValue::Four;
	case '5': return CardValue::Five;
	case '6': return CardValue::Six;
	case '7': return CardValue::Seven;
	case '8': return CardValue::Eight;
	case '9': return CardValue::Nine;
	case '0': return CardValue::Ten;
	case 'j':
	case 'J': return CardValue::Jack;
	case 'q':
	case 'Q': return CardValue::Queen;
	case 'k':
	case 'K': return CardValue::King;
	case 'a':
	case 'A': return CardValue::Ace;
	default:
		return static_cast<CardValue>(-1);
	}
}

struct Card
{
	Card() : color(CardColor::Spade), value(CardValue::Deuce) {}
	Card(uint8_t v) : color(static_cast<CardColor>(v & 3)), value(static_cast<CardValue>(v >> 2)) {}
	Card(const char* card) : color(CharToCardColor(card[1])), value(CharToCardValue(card[0])) {}
	CardColor color : 2;
	CardValue value : 4;
	bool operator<(Card c) const
	{
		return value < c.value;
	}
	bool operator==(Card c) const
	{
		return color == c.color && value == c.value;
	}
	std::string ToString() const
	{
		std::string s;
		switch (value)
		{
		case CardValue::Deuce: s += "2"; break;
		case CardValue::Three: s += "3"; break;
		case CardValue::Four: s += "4"; break;
		case CardValue::Five: s += "5"; break;
		case CardValue::Six: s += "6"; break;
		case CardValue::Seven: s += "7"; break;
		case CardValue::Eight: s += "8"; break;
		case CardValue::Nine: s += "9"; break;
		case CardValue::Ten: s += "10"; break;
		case CardValue::Jack: s += "J"; break;
		case CardValue::Queen: s += "Q"; break;
		case CardValue::King: s += "K"; break;
		case CardValue::Ace: s += "A"; break;
		}
		switch (color)
		{
		case CardColor::Spade: s += "s"; break;
		case CardColor::Heart: s += "h"; break;
		case CardColor::Diamond: s += "d"; break;
		case CardColor::Club: s += "c"; break;
		}
		return s;
	}
};

std::string ToString(const Card* cards, int numCards, bool reverse = true)
{
	std::string s;
	if (reverse)
	{
		for (int32_t i = numCards; i--;)
		{
			s += cards[i].ToString();
			if (i > 0)
			{
				s += ", ";
			}
		}
	}
	else
	{
		for (int32_t i = 0; i < numCards; ++i)
		{
			s += cards[i].ToString();
			if (i < numCards - 1)
			{
				s += ", ";
			}
		}
	}
	return s;
}

std::string ToString(HandType type)
{
	switch (type)
	{
	case HandType::HighCard: return "HighCard";
	case HandType::OnePair: return "OnePair";
	case HandType::TwoPair: return "TwoPair";
	case HandType::ThreeOfAKind: return "ThreeOfAKind";
	case HandType::Straight: return "Straight";
	case HandType::Flush: return "Flush";
	case HandType::FullHouse: return "FullHouse";
	case HandType::FourOfAKind: return "FourOfAKind";
	case HandType::StraightFlush: return "StraightFlush";
	}
	return "";
}

char* g_den[9] = {"HighCard", "OnePair", "TwoPair", "ThreeOfAKind", "Straight", "Flush", "FullHouse", "FourOfAKind", "StraightFlush"};

bool CardLess(uint8_t c1, uint8_t c2)
{
	return c1 < c2;
}

HandType GetHandType(const Card cards[5])
{
	if (cards[0].color == cards[1].color &&
		cards[0].color == cards[2].color &&
		cards[0].color == cards[3].color &&
		cards[0].color == cards[4].color)
	{
		//hand is Flush or StraightFlush
		if ((static_cast<uint8_t>(cards[4].value) - static_cast<uint8_t>(cards[0].value) == 4) ||
			(cards[4].value == CardValue::Ace && cards[3].value == CardValue::Five))
			return HandType::StraightFlush;
		return HandType::Flush;
	}
	uint8_t nmaxofakind = 1;
	uint8_t cons = 1;
	for (uint8_t i = 1; i < 5; i++)
		if (cards[i].value == cards[i - 1].value)
			cons++;
		else {
			if (nmaxofakind < cons)
				nmaxofakind = cons;
			cons = 1;
		}
		if (nmaxofakind < cons)
			nmaxofakind = cons;
		if (nmaxofakind == 4)
			return HandType::FourOfAKind;
		if (nmaxofakind == 3) {
			// hand is FullHouse or ThreeOfAKind
			if ((cards[0].value == cards[1].value) && (cards[3].value == cards[4].value))
				return HandType::FullHouse;
			return HandType::ThreeOfAKind;
		}
		if (nmaxofakind == 2) {
			//hand is TwoPair or OnePair
			uint8_t nequals = 0;
			for (uint8_t i = 1; i < 5; i++)
				if (cards[i].value == cards[i - 1].value)
					nequals++;
			if (nequals & 1)
				return HandType::OnePair;
			return HandType::TwoPair;
		}
		//hand is HighCard or Straight
		if ((static_cast<uint8_t>(cards[4].value) - static_cast<uint8_t>(cards[0].value) == 4) ||
			(cards[4].value == CardValue::Ace && cards[3].value == CardValue::Five))
			return HandType::Straight;
		return HandType::HighCard;
}

HandType GetHandType(uint8_t cards[5])
{
	uint8_t color = cards[0] & 3;
	if (color == (cards[1] & 3) && color == (cards[2] & 3) && color == (cards[3] & 3) && color == (cards[4] & 3))
	{
		//hand is Flush or StraightFlush
		if( ((cards[4] >> 2) - (cards[0] >> 2) == 4) || ((cards[4] >> 2 == 12) && (cards[3] >> 2 == 3)) )
			return HandType::StraightFlush;
		return HandType::Flush;
	}
	byte nmaxofakind = 1;
	byte cons = 1;
	for( byte i = 1; i < 5; i++ )
		if( cards[i] >> 2 == cards[i-1] >> 2 )
			cons++;
		else{
			if(nmaxofakind < cons)
				nmaxofakind = cons;
			cons = 1;
		}
	if(nmaxofakind < cons)
		nmaxofakind = cons;
	if( nmaxofakind == 4 )
		return HandType::FourOfAKind;
	if( nmaxofakind == 3 ){
		// hand is FullHouse or ThreeOfAKind
		if( (cards[0] >> 2 == cards[1] >> 2) && (cards[3] >> 2 == cards[4] >> 2) )
			return HandType::FullHouse;
		return HandType::ThreeOfAKind;
	}
	if( nmaxofakind == 2 ){
		//hand is TwoPair or OnePair
		byte nequals = 0;
		for( byte i = 1; i < 5; i++ )
			if( cards[i] >> 2 == cards[i-1] >> 2 )
				nequals++;
		if( nequals & 1 )
			return HandType::OnePair;
		return HandType::TwoPair;
	}
	//hand is HighCard or Straight
	if( ((cards[4] >> 2) - (cards[0] >> 2) == 4) || ((cards[4] >> 2 == 12) && (cards[3] >> 2 == 3)) )
		return HandType::Straight;
	return HandType::HighCard;
}

int CompareHands( byte hand1[5], byte hand2[5] )
{
	HandType ht1 = GetHandType(hand1);
	HandType ht2 = GetHandType(hand2);

	if( ht1 > ht2 )
		return 1;
	if( ht1 < ht2 )
		return -1;

	byte v1, v2;

	if( ht1 == HandType::StraightFlush || ht1 == HandType::Straight ){
		v1 = hand1[0] >> 2;
		v2 = hand2[0] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;
		v1 = hand1[4] >> 2;
		v2 = hand2[4] >> 2;
		if (v1 < v2)
			return 1;
		if (v1 > v2)
			return -1;
		return 0;
		return 0;
	}

	if( ht1 == HandType::FourOfAKind || ht1 == HandType::FullHouse ){
		v1 = hand1[2] >> 2;
		v2 = hand2[2] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;
		if( (hand1[4] >> 2) == v1 )
			v1 = hand1[0] >> 2;
		else
			v1 = hand1[4] >> 2;
		if( (hand2[4] >> 2) == v2 )
			v2 = hand2[0] >> 2;
		else
			v2 = hand2[4] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;
		return 0;
	}

	if( ht1 == HandType::Flush ){
		byte i = 4;
		v1 = hand1[i] >> 2;
		v2 = hand2[i] >> 2;
		while( v1 == v2 && i > 0 ){
			i--;
			v1 = hand1[i] >> 2;
			v2 = hand2[i] >> 2;
		}
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;
		return 0;
	}

	if( ht1 == HandType::ThreeOfAKind ){
		v1 = hand1[2] >> 2;
		v2 = hand2[2] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		if( (hand1[2] >> 2) != (hand1[4] >> 2) )
			v1 = hand1[4] >> 2;
		else
			v1 = hand1[1] >> 2;
		if( (hand2[2] >> 2) != (hand2[4] >> 2) )
			v2 = hand2[4] >> 2;
		else
			v2 = hand2[1] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		if( (hand1[2] >> 2) != (hand1[0] >> 2) )
			v1 = hand1[0] >> 2;
		else
			v1 = hand1[3] >> 2;
		if( (hand2[2] >> 2) != (hand2[0] >> 2) )
			v2 = hand2[0] >> 2;
		else
			v2 = hand2[3] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		return 0;
	}

	if( ht1 == HandType::TwoPair ){
		v1 = hand1[3] >> 2;
		v2 = hand2[3] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		v1 = hand1[1] >> 2;
		v2 = hand2[1] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		if( (hand1[3] >> 2) != (hand1[4] >> 2) )
			v1 = hand1[4] >> 2;
		else if( (hand1[1] >> 2) != (hand1[0] >> 2) )
			v1 = hand1[0] >> 2;
		else
			v1 = hand1[2] >> 2;
		if( (hand2[3] >> 2) != (hand2[4] >> 2) )
			v2 = hand2[4] >> 2;
		else if( (hand2[1] >> 2) != (hand2[0] >> 2) )
			v2 = hand2[0] >> 2;
		else
			v2 = hand2[2] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		return 0;
	}

	if( ht1 == HandType::OnePair ){
		byte i1 = 0, i2 = 0;
		while( i1 < 4 ){
			if( (hand1[i1] >> 2) == (hand1[i1+1] >> 2) )
				break;
			i1++;
		}
		while( i2 < 4 ){
			if( (hand2[i2] >> 2) == (hand2[i2+1] >> 2) )
				break;
			i2++;
		}
		v1 = hand1[i1] >> 2;
		v2 = hand2[i2] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		byte j1 = (i1 == 3) ? 2 : 4;
		byte j2 = (i2 == 3) ? 2 : 4;
		v1 = hand1[j1] >> 2;
		v2 = hand2[j2] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		j1 = (i1 > 1) ? 1 : 3;
		j2 = (i2 > 1) ? 1 : 3;
		v1 = hand1[j1] >> 2;
		v2 = hand2[j2] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		j1 = (i1 == 0) ? 2 : 0;
		j2 = (i2 == 0) ? 2 : 0;
		v1 = hand1[j1] >> 2;
		v2 = hand2[j2] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;

		return 0;
	}

	if( ht1 == HandType::HighCard ){
		for (int32_t i = 5; i--;) {
			v1 = hand1[i] >> 2;
			v2 = hand2[i] >> 2;
			if( v1 > v2 )
				return 1;
			if( v1 < v2 )
				return -1;
		}

		return 0;
	}
}

int CompareHands(const Card hand1[5], const Card hand2[5])
{
	HandType ht1 = GetHandType(hand1);
	HandType ht2 = GetHandType(hand2);

	if (ht1 > ht2)
		return 1;
	if (ht1 < ht2)
		return -1;

	CardValue v1, v2;

	switch (ht1)
	{
	case HandType::HighCard:
		{
			for (int32_t i = 5; i--;) {
				v1 = hand1[i].value;
				v2 = hand2[i].value;
				if (v1 > v2)
					return 1;
				if (v1 < v2)
					return -1;
			}

			return 0;
		}
	case HandType::OnePair:
		{
			byte i1 = 0, i2 = 0;
			while (i1 < 4) {
				if ((hand1[i1].value) == (hand1[i1 + 1].value))
					break;
				i1++;
			}
			while (i2 < 4) {
				if ((hand2[i2].value) == (hand2[i2 + 1].value))
					break;
				i2++;
			}
			v1 = hand1[i1].value;
			v2 = hand2[i2].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			byte j1 = (i1 == 3) ? 2 : 4;
			byte j2 = (i2 == 3) ? 2 : 4;
			v1 = hand1[j1].value;
			v2 = hand2[j2].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			j1 = (i1 > 1) ? 1 : 3;
			j2 = (i2 > 1) ? 1 : 3;
			v1 = hand1[j1].value;
			v2 = hand2[j2].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			j1 = (i1 == 0) ? 2 : 0;
			j2 = (i2 == 0) ? 2 : 0;
			v1 = hand1[j1].value;
			v2 = hand2[j2].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			return 0;
		}
	case HandType::TwoPair:
		{
			v1 = hand1[3].value;
			v2 = hand2[3].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			v1 = hand1[1].value;
			v2 = hand2[1].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			if ((hand1[3].value) != (hand1[4].value))
				v1 = hand1[4].value;
			else if ((hand1[1].value) != (hand1[0].value))
				v1 = hand1[0].value;
			else
				v1 = hand1[2].value;
			if ((hand2[3].value) != (hand2[4].value))
				v2 = hand2[4].value;
			else if ((hand2[1].value) != (hand2[0].value))
				v2 = hand2[0].value;
			else
				v2 = hand2[2].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			return 0;
		}
	case HandType::ThreeOfAKind:
		{
			v1 = hand1[2].value;
			v2 = hand2[2].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			if ((hand1[2].value) != (hand1[4].value))
				v1 = hand1[4].value;
			else
				v1 = hand1[1].value;
			if ((hand2[2].value) != (hand2[4].value))
				v2 = hand2[4].value;
			else
				v2 = hand2[1].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			if ((hand1[2].value) != (hand1[0].value))
				v1 = hand1[0].value;
			else
				v1 = hand1[3].value;
			if ((hand2[2].value) != (hand2[0].value))
				v2 = hand2[0].value;
			else
				v2 = hand2[3].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			return 0;
		}
	case HandType::Straight:
	case HandType::StraightFlush:
		{
			v1 = hand1[0].value;
			v2 = hand2[0].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;
			v1 = hand1[4].value;
			v2 = hand2[4].value;
			if (v1 < v2)
				return 1;
			if (v1 > v2)
				return -1;

			return 0;
		}
	case HandType::Flush:
		{
			byte i = 4;
			v1 = hand1[i].value;
			v2 = hand2[i].value;
			while (v1 == v2 && i > 0) {
				i--;
				v1 = hand1[i].value;
				v2 = hand2[i].value;
			}
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			return 0;
		}
	case HandType::FullHouse:
	case HandType::FourOfAKind:
		{
			v1 = hand1[2].value;
			v2 = hand2[2].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;
			if ((hand1[4].value) == v1)
				v1 = hand1[0].value;
			else
				v1 = hand1[4].value;
			if ((hand2[4].value) == v2)
				v2 = hand2[0].value;
			else
				v2 = hand2[4].value;
			if (v1 > v2)
				return 1;
			if (v1 < v2)
				return -1;

			return 0;
		}
	}
}

void sortCards( byte cards[5] )
{
	byte temp;
	for( byte i = 0; i < 4; i++ )
		for( byte j = i+1; j < 5; j++ )
			if( cards[i] > cards[j] ){
				temp = cards[i];
				cards[i] = cards[j];
				cards[j] = temp;
			}
}

void PrintSymbol( char* str, byte card )
{
	byte val = card >> 2;
	char cval = val + 2;

	str[0] = ' ';

	if( val < 8 )
		str[1] = '0' + cval;
	else if( val == 8 ){
		str[0] = '1';
		str[1] = '0';
	}
	else
	{
		switch( val ){
			case  9: str[1] = 'J'; break;
			case 10: str[1] = 'Q'; break;
			case 11: str[1] = 'K'; break;
			case 12: str[1] = 'A'; break;
		}
	}

	switch( card & 3 ){
		case 0: str[2] = 's'; break;
		case 1: str[2] = 'h'; break;
		case 2: str[2] = 'd'; break;
		case 3: str[2] = 'c'; break;
	}
}

void PrintHand( char* str, byte cards[5] )
{
	PrintSymbol( str, cards[0] );
	str[3] = ' ';
	PrintSymbol( str + 4, cards[1] );
	str[7] = ' ';
	PrintSymbol( str + 8, cards[2] );
	str[11] = ' ';
	PrintSymbol( str + 12, cards[3] );
	str[15] = ' ';
	PrintSymbol( str + 16, cards[4] );
}

void SaveAllHandsToFile()
{
	byte cards[5];
	HandType ht;
	int kHand = 0;
	char sHand[19];
	int stats[9];

	memset( stats, 0, 9 * sizeof(int) );

	FILE *f;
	//fopen_s( &f, "C:\\poker.txt", "wt" );

	for( byte k0 = 0; k0 < 48; k0++ )
	{
		cards[0] = k0;
		for( byte k1 = k0+1; k1 < 49; k1++ )
		{
			cards[1] = k1;
			for( byte k2 = k1+1; k2 < 50; k2++ )
			{
				cards[2] = k2;
				for( byte k3 = k2+1; k3 < 51; k3++ )
				{
					cards[3] = k3;
					for( byte k4 = k3+1; k4 < 52; k4++ )
					{
						cards[4] = k4;
						ht = GetHandType( cards );
						stats[(int)ht]++;
						//PrintHand( sHand, cards );
						//fprintf( f, "%.7i %s   %s\n", kHand++, sHand, g_den[ht] );
					}
				}
			}
		}
	}

	for( byte i = 0; i < 9; i++ )
		printf( "%d %d\n", i, stats[i] );

	int x;
	scanf_s("%d", &x);

	//fclose( f );
}

byte DealCard( byte cardsDealt[], byte nCardsDealt )
{
	byte card, i;
	while( 1 ){
		card = rand() % 52;
		for( i = 0; i < nCardsDealt; i++ )
			if( card == cardsDealt[i] )
				break;
		if( i == nCardsDealt ){
			cardsDealt[nCardsDealt] = card;
			return card;
		}
	}
	return 0;
}

void GetBestHand( byte cards[], byte nCards, byte bestHand[] )
{
	byte idx[5] = { 0, 1, 2, 3, 4 };
	bestHand[0] = cards[idx[0]];
	bestHand[1] = cards[idx[1]];
	bestHand[2] = cards[idx[2]];
	bestHand[3] = cards[idx[3]];
	bestHand[4] = cards[idx[4]];
	sortCards( bestHand );
	byte newHand[5];
	while( idx[0] < nCards - 5 )
	{
		for( byte i = 4; i >= 0; i-- )
			if( idx[i] < nCards - 5 + i ){
				idx[i]++;
				for( byte j = i+1; j < 5; j++ )
					idx[j] = idx[j-1] + 1;
				break;
			}
		newHand[0] = cards[idx[0]];
		newHand[1] = cards[idx[1]];
		newHand[2] = cards[idx[2]];
		newHand[3] = cards[idx[3]];
		newHand[4] = cards[idx[4]];
		sortCards( newHand );

		if( CompareHands( bestHand, newHand ) == -1 ){
			bestHand[0] = newHand[0];
			bestHand[1] = newHand[1];
			bestHand[2] = newHand[2];
			bestHand[3] = newHand[3];
			bestHand[4] = newHand[4];
		}
	}
}

void DecideBeforeDeal()
{

}

void DecideAfterFlop( byte hand[], byte table[], float& fWin, float& fDraw )
{
	byte otherHand[2];
	byte bestHand1[5];
	byte bestHand2[5];
	byte cards[7];
	cards[2] = table[0];
	cards[3] = table[1];
	cards[4] = table[2];
	int nTotalHands = 0;
	int nWonHands = 0;
	int nDrawHands = 0;
	for( byte k0 = 0; k0 < 51; k0++ )
	{
		if( hand[0] == k0 || hand[1] == k0 || table[0] == k0 || table[1] == k0 || table[2] == k0 )
			continue;
		otherHand[0] = k0;
		for( byte k1 = k0+1; k1 < 52; k1++ )
		{
			if( hand[0] == k1 || hand[1] == k1 || table[0] == k1 || table[1] == k1 || table[2] == k1 )
				continue;
			otherHand[1] = k1;
			for( byte turn = 0; turn < 51; turn++ )
			{
				if( hand[0] == turn || hand[1] == turn || otherHand[0] == turn || otherHand[1] == turn ||
					table[0] == turn || table[1] == turn || table[2] == turn )
					continue;
				cards[5] = turn;
				for( byte river = turn+1; river < 52; river++ )
				{
					if( hand[0] == river || hand[1] == river || otherHand[0] == river || otherHand[1] == river ||
						table[0] == river || table[1] == river || table[2] == river )
						continue;
					cards[6] = river;
					cards[0] = hand[0];
					cards[1] = hand[1];
					GetBestHand( cards, 7, bestHand1 );
					cards[0] = otherHand[0];
					cards[1] = otherHand[1];
					GetBestHand( cards, 7, bestHand2 );
					int res = CompareHands( bestHand1, bestHand2 );
					nTotalHands++;
					if( res == 1 )
						nWonHands++;
					else if( res == 0 )
						nDrawHands++;
				}
			}
		}
	}
	fWin = (float)nWonHands / nTotalHands;
	fDraw = (float)nDrawHands / nTotalHands;
}

void DecideAfterFlop2( byte hand[], byte table[], float& fWin, float& fDraw, const int nTries )
{
	byte otherHand[2];
	byte bestHand1[5];
	byte bestHand2[5];
	byte cards[7];
	cards[2] = table[0];
	cards[3] = table[1];
	cards[4] = table[2];
	const int nTotalHands = nTries;
	int nWonHands = 0;
	int nDrawHands = 0;
	for( int k = 0; k < nTotalHands; k++ )
	{
		otherHand[0] = rand() % 52;
		while( hand[0] == otherHand[0] || hand[1] == otherHand[0] || table[0] == otherHand[0] || table[1] == otherHand[0] || table[2] == otherHand[0] )
			otherHand[0] = rand() % 52;

		otherHand[1] = rand() % 52;
		while( hand[0] == otherHand[1] || hand[1] == otherHand[1] || table[0] == otherHand[1] || table[1] == otherHand[1] || table[2] == otherHand[1] ||
			otherHand[0] == otherHand[1] )
			otherHand[1] = rand() % 52;

		cards[5] = rand() % 52;
		while( hand[0] == cards[5] || hand[1] == cards[5] || table[0] == cards[5] || table[1] == cards[5] || table[2] == cards[5] ||
			otherHand[0] == cards[5] || otherHand[1] == cards[5] )
			cards[5] = rand() % 52;

		cards[6] = rand() % 52;
		while( hand[0] == cards[6] || hand[1] == cards[6] || table[0] == cards[6] || table[1] == cards[6] || table[2] == cards[6] ||
			otherHand[0] == cards[6] || otherHand[1] == cards[6] || cards[5] == cards[6] )
			cards[6] = rand() % 52;

		cards[0] = hand[0];
		cards[1] = hand[1];
		GetBestHand( cards, 7, bestHand1 );
		cards[0] = otherHand[0];
		cards[1] = otherHand[1];
		GetBestHand( cards, 7, bestHand2 );
		int res = CompareHands( bestHand1, bestHand2 );
		if( res == 1 )
			nWonHands++;
		else if( res == 0 )
			nDrawHands++;
	}
	fWin = (float)nWonHands / nTotalHands;
	fDraw = (float)nDrawHands / nTotalHands;
}

void DecideAfterTurn( byte hand[], byte table[], float& fWin, float& fDraw )
{
	byte otherHand[2];
	byte bestHand1[5];
	byte bestHand2[5];
	byte cards[7];
	cards[2] = table[0];
	cards[3] = table[1];
	cards[4] = table[2];
	cards[5] = table[3];
	int nTotalHands = 0;
	int nWonHands = 0;
	int nDrawHands = 0;
	for( byte k0 = 0; k0 < 51; k0++ )
	{
		if( hand[0] == k0 || hand[1] == k0 || table[0] == k0 || table[1] == k0 || table[2] == k0 || table[3] == k0 )
			continue;
		otherHand[0] = k0;
		for( byte k1 = k0+1; k1 < 52; k1++ )
		{
			if( hand[0] == k1 || hand[1] == k1 || table[0] == k1 || table[1] == k1 || table[2] == k1 || table[3] == k1 )
				continue;
			otherHand[1] = k1;
			for( byte river = 0; river < 52; river++ )
			{
				if( hand[0] == river || hand[1] == river || otherHand[0] == river || otherHand[1] == river ||
					table[0] == river || table[1] == river || table[2] == river || table[3] == river )
					continue;
				cards[6] = river;
				cards[0] = hand[0];
				cards[1] = hand[1];
				GetBestHand( cards, 7, bestHand1 );
				cards[0] = otherHand[0];
				cards[1] = otherHand[1];
				GetBestHand( cards, 7, bestHand2 );
				int res = CompareHands( bestHand1, bestHand2 );
				nTotalHands++;
				if( res == 1 )
					nWonHands++;
				else if( res == 0 )
					nDrawHands++;
			}
		}
	}
	fWin = (float)nWonHands / nTotalHands;
	fDraw = (float)nDrawHands / nTotalHands;
}

void DecideAfterRiver( byte hand[], byte table[], float& fWin, float& fDraw )
{
	byte otherHand[2];
	byte bestHand1[5];
	byte bestHand2[5];
	byte cards[7];
	cards[2] = table[0];
	cards[3] = table[1];
	cards[4] = table[2];
	cards[5] = table[3];
	cards[6] = table[4];
	int nTotalHands = 0;
	int nWonHands = 0;
	int nDrawHands = 0;
	for( byte k0 = 0; k0 < 51; k0++ )
	{
		if( hand[0] == k0 || hand[1] == k0 || table[0] == k0 || table[1] == k0 || table[2] == k0 || table[3] == k0 )
			continue;
		otherHand[0] = k0;
		for( byte k1 = k0+1; k1 < 52; k1++ )
		{
			if( hand[0] == k1 || hand[1] == k1 || table[0] == k1 || table[1] == k1 || table[2] == k1 || table[3] == k1 )
				continue;
			otherHand[1] = k1;

			cards[0] = hand[0];
			cards[1] = hand[1];
			GetBestHand( cards, 7, bestHand1 );
			cards[0] = otherHand[0];
			cards[1] = otherHand[1];
			GetBestHand( cards, 7, bestHand2 );
			int res = CompareHands( bestHand1, bestHand2 );
			nTotalHands++;
			if( res == 1 )
				nWonHands++;
			else if( res == 0 )
				nDrawHands++;
		}
	}
	fWin = (float)nWonHands / nTotalHands;
	fDraw = (float)nDrawHands / nTotalHands;
}

void RefreshComputerStack( int value )
{
	mvprintw(0,0,"                                                                        ");
	mvprintw(0,0,"Computer stack: %d$", value);
	refresh();
}

void RefreshHumanStack( int value )
{
	mvprintw(1,0,"                                                                        ");
	mvprintw(1,0,"Your stack: %d$", value);
	refresh();
}

void RefreshPot( int value )
{
	mvprintw(2,0,"                                                                        ");
	mvprintw(2,0,"Pot: %d$", value);
	refresh();
}

void GameOn()
{
	srand( GetTickCount() );

	byte cards[2][2];
	byte table[5];
	byte cardsDealt[9];

	int stack[2] = { 1000, 1000 };
	int pot, dStack, minBet, count = 0, logpos = 6;

	float fWin, fDraw;

	initscr();

	while( 1 )
	{
		getch();
		erase();

		logpos = 6;

		pot = 0;

		cards[0][0] = DealCard( cardsDealt, 0 );
		cards[1][0] = DealCard( cardsDealt, 1 );
		cards[0][1] = DealCard( cardsDealt, 2 );
		cards[1][1] = DealCard( cardsDealt, 3 );

		dStack = MIN( stack[0], 10 );
		pot += dStack;
		stack[0] -= dStack;
		dStack = MIN( stack[1], 10 );
		pot += dStack;
		stack[1] -= dStack;

		table[0] = DealCard( cardsDealt, 4 );
		table[1] = DealCard( cardsDealt, 5 );
		table[2] = DealCard( cardsDealt, 6 );

		RefreshComputerStack( stack[0] );
		RefreshHumanStack( stack[1] );
		RefreshPot( pot );

		mvprintw(3,0,"Table: ");

		char sCard[4];
		sCard[3] = 0;
		PrintSymbol( sCard, cards[1][0] );
		mvprintw(4,0,"Your Hand: %s", sCard);
		PrintSymbol( sCard, cards[1][1] );
		mvprintw(4,14," %s", sCard);

		bool ended;

		for( int step = 0; step < 3; step++ )
		{
			if( step == 0 ){
				PrintSymbol( sCard, table[0] );
				mvprintw(3,7,"%s",sCard);
				PrintSymbol( sCard, table[1] );
				mvprintw(3,11,"%s",sCard);
				PrintSymbol( sCard, table[2] );
				mvprintw(3,15,"%s",sCard);
				refresh();

				DecideAfterFlop( cards[0], table, fWin, fDraw );
			}
			if( step == 1 ){
				table[3] = DealCard( cardsDealt, 7 );

				PrintSymbol( sCard, table[0] );
				mvprintw(3,7,"%s",sCard);
				PrintSymbol( sCard, table[1] );
				mvprintw(3,11,"%s",sCard);
				PrintSymbol( sCard, table[2] );
				mvprintw(3,15,"%s",sCard);
				PrintSymbol( sCard, table[3] );
				mvprintw(3,19,"%s",sCard);
				refresh();

				DecideAfterTurn( cards[0], table, fWin, fDraw );
			}
			if( step == 2 ){
				table[4] = DealCard( cardsDealt, 8 );

				PrintSymbol( sCard, table[0] );
				mvprintw(3,7,"%s",sCard);
				PrintSymbol( sCard, table[1] );
				mvprintw(3,11,"%s",sCard);
				PrintSymbol( sCard, table[2] );
				mvprintw(3,15,"%s",sCard);
				PrintSymbol( sCard, table[3] );
				mvprintw(3,19,"%s",sCard);
				PrintSymbol( sCard, table[4] );
				mvprintw(3,23,"%s",sCard);
				refresh();

				DecideAfterRiver( cards[0], table, fWin, fDraw );
			}

			if( stack[0] == 0 || stack[1] == 0 )
				continue;

			minBet = 0;

			if( count & 1 ){
				float r = (float)rand() / (1 << 15);
				int bet;
				if( r <= fWin + fDraw ){
					bet = r / (fWin + fDraw) * pot;
					bet = MAX(bet, 10);
				}
				else
					bet = 0;
				if( bet > stack[0] )
					bet = stack[0];
				stack[0] -= bet;
				pot += bet;
				minBet = bet;
				mvprintw(logpos++,0,"Computer bets %d.", bet);
				RefreshComputerStack( stack[0] );
				RefreshPot( pot );
			}

			ended = false;

			while( 1 ){
				int humanBet;
				while( 1 ){
					mvprintw(logpos++,0,"Place bet (minimum %d): ",minBet);
					refresh();
					scanw("%d",&humanBet);
					refresh();
					if( humanBet == -1 )
						break;
					if( humanBet == stack[1] )
						break;
					if( stack[0] == 0 && humanBet != minBet )
						continue;
					if( minBet == 0 ){
						if( humanBet == 0 )
							break;
						if( humanBet >= 10 && humanBet < stack[1] )
							break;
					}
					if( minBet != 0 ){
						if( humanBet == minBet )
							break;
						if( humanBet >= 2 * minBet && humanBet < stack[1] )
							break;
					}
				}
				if( humanBet == -1 ){
					mvprintw(logpos++,0,"You folded.");
					ended = true;
					stack[0] += pot;
					break;
				}
				humanBet = MIN( humanBet, stack[1] );
				stack[1] -= humanBet;
				pot += humanBet;
				minBet = humanBet - minBet;
				if( minBet < 0 ){
					stack[0] -= minBet;
					pot += minBet;
					RefreshComputerStack( stack[0] );
				}
				if( minBet )
					mvprintw(logpos++,0,"You bet %d.", humanBet);
				else
					mvprintw(logpos++,0,"You call %d.", humanBet);
				RefreshHumanStack( stack[1] );
				RefreshPot( pot );

				if( minBet < 0 )
					break;
				if( minBet == 0 && humanBet != 0 )
					break;
				if( minBet == 0 && humanBet == 0 && (count & 1) )
					break;

				if( minBet == 0 ){
					float r = (float)rand() / (1 << 15);
					int bet;
					if( r <= fWin + fDraw ){
						bet = r / (fWin + fDraw) * pot;
						bet = MAX(bet, 10);
					}
					else{
						mvprintw(logpos++,0,"Computer checks.");
						refresh();
						break;
					}
					if( bet > stack[0] )
						bet = stack[0];
					stack[0] -= bet;
					pot += bet;
					minBet = bet;
					mvprintw(logpos++,0,"Computer bets %d.", bet);
				}
				else{
					if( (float)minBet / (minBet + pot) > fWin + fDraw ){
						mvprintw(logpos++,0,"Computer folds.");
						ended = true;
						stack[1] += pot;
						break;
					}
					int bet;
					float r = (float)rand() / (1 << 15);
					if( r > fWin + fDraw || stack[1] == 0 || stack[0] < minBet ){
						bet = MIN(minBet, stack[0]);
						if( bet < minBet ){
							pot -= minBet - bet;
							stack[1] += minBet - bet;
							RefreshHumanStack( stack[1] );
						}
						stack[0] -= bet;
						pot += bet;
						mvprintw(logpos++,0,"Computers calls %d.", bet);
						RefreshComputerStack( stack[0] );
						RefreshPot( pot );
						break;
					}
					r = r / fWin + fDraw;
					if( r > .9f )
						bet = stack[0];
					else if( r > .7f )
						bet = 2 * minBet;
					else
						bet = 2 * minBet + (2 * r * minBet);
					bet = MIN( bet, stack[0] );
					stack[0] -= bet;
					pot += bet;
					minBet = bet - minBet;
					mvprintw(logpos++,0,"Computer raises %d.", minBet);
				}
				RefreshComputerStack( stack[0] );
				RefreshPot( pot );
			}

			if( ended )
				break;
		}

		if( !ended ){
			byte tstcards[7];
			byte bestHand1[5];
			byte bestHand2[5];
			tstcards[2] = table[0];
			tstcards[3] = table[1];
			tstcards[4] = table[2];
			tstcards[5] = table[3];
			tstcards[6] = table[4];
			tstcards[0] = cards[0][0];
			tstcards[1] = cards[0][1];
			GetBestHand( tstcards, 7, bestHand1 );
			tstcards[0] = cards[1][0];
			tstcards[1] = cards[1][1];
			GetBestHand( tstcards, 7, bestHand2 );
			int res = CompareHands( bestHand1, bestHand2 );

			if( res == 1 ){
				mvprintw(logpos++,0,"Computer wins with a %s.", g_den[(int)GetHandType(bestHand1)] );
				PrintSymbol( sCard, cards[0][0] );
				mvprintw(logpos++,0,"Computer had: %s ", sCard);
				PrintSymbol( sCard, cards[0][1] );
				printw("%s\n", sCard);
				stack[0] += pot;
			}
			else if( res == -1 ){
				mvprintw(logpos++,0,"Human wins with a %s.", g_den[(int)GetHandType(bestHand2)] );
				PrintSymbol( sCard, cards[0][0] );
				mvprintw(logpos++,0,"Computer had: %s ", sCard);
				PrintSymbol( sCard, cards[0][1] );
				printw("%s\n", sCard);
				stack[1] += pot;
			}
			else{
				mvprintw(logpos++,0,"Both players tie a %s.", g_den[(int)GetHandType(bestHand2)] );
				PrintSymbol( sCard, cards[0][0] );
				mvprintw(logpos++,0,"Computer had: %s ", sCard);
				PrintSymbol( sCard, cards[0][1] );
				printw("%s\n", sCard);
				stack[0] += pot / 2;
				stack[1] += pot / 2;
			}
		}

		RefreshComputerStack( stack[0] );
		RefreshHumanStack( stack[1] );
		RefreshPot( pot );

		count++;

		if( stack[0] == 0 ){
			mvprintw(logpos++,0,"You win!\n");
			refresh();
			break;
		}
		else if( stack[1] == 0 ){
			mvprintw(logpos++,0,"You lose!\n");
			refresh();
			break;
		}
	}

	int x;
	scanf( "%d", &x);
}

DWORD WINAPI tetsf( LPVOID zzz )
{
	double x = 0;
	while(x < 20000000)
	{
		x += fabs( cos( (double)rand() ) );
	}
	return 0;
}

template<typename T>
std::vector<T> GetVectorForPermutation()
{
	const auto n = 50;
	const auto k = 6;
	std::vector<T> v(n);
	std::fill(v.begin(), v.end() - k, T(0));
	std::fill(v.begin() + n - k, v.end(), T(1));
	return v;
}

void Replace(Card dstHand[5], const Card srcHand[5])
{
	reinterpret_cast<int32_t&>(dstHand[0]) = reinterpret_cast<const int32_t&>(srcHand[0]);
	dstHand[4] = srcHand[4];
}

void ReplaceIfBetter(Card targetHand[5], const Card candidateHand[5])
{
	if (CompareHands(candidateHand, targetHand) == 1)
	{
		Replace(targetHand, candidateHand);
	}
}

void GetBestHand(const std::vector<Card>& cards, Card bestHand[5])
{
	const int32_t numHandCards = 5;
	const int32_t numCards = cards.size();
	std::vector<int32_t> handPicker(numCards);
	std::fill(handPicker.begin(), handPicker.end() - numHandCards, 0);
	std::fill(handPicker.end() - numHandCards, handPicker.end(), 1);

	for (int32_t k = 0; k < numHandCards; ++k)
	{
		bestHand[k] = cards[k + numCards - numHandCards];
	}

	Card hand[numHandCards];
	int32_t hc;
	while (std::next_permutation(handPicker.begin(), handPicker.end()))
	{
		hc = 0;
		for (int32_t k = 0; k < numCards; ++k)
		{
			if (handPicker[k] != 0)
			{
				hand[hc++] = cards[k];
			}
		}
		//printf("Hand type: %13s Hand: %s\n", ToString(GetHandType(&hand[0])).c_str(), ToString(&hand[0], 5).c_str());
		ReplaceIfBetter(bestHand, hand);
	}
}

uintmax_t Combination(uint32_t n, uint32_t k)
{
	if (n - k < k)
	{
		return Combination(n, n - k);
	}
	else
	{
		uintmax_t result = 1;
		for (uint32_t i = 0; i < k; ++i)
		{
			result *= (n - i);
		}
		for (uint32_t i = 2; i <= k; ++i)
		{
			result /= i;
		}
		return result;
	}
}


struct Chances
{
	Chances() : total(0), winning(0), split(0) {}
	Chances& operator+=(const Chances& c) { total += c.total; winning += c.winning; split += c.split; return *this; }
	const Chances operator+(const Chances& c) const { return Chances(*this) += c; }

	uintmax_t total;
	uintmax_t winning;
	uintmax_t split;
};

static const int32_t numThreads = 8;
static const int32_t handsPerThread = 2500;

template <typename T>
void FastCopy(
	decltype(declval<std:vector<T>>().cbegin())& srcBegin, 
	decltype(declval<std:vector<T>>().cend())& srcEnd, 
	decltype(declval<std:vector<T>>().begin())& dst)
{
	static auto copy = [](auto v, const T* src, T* dst) {
		typedef decltype(v) copy_t;
		reinterpret_cast<copy_t&>(*dst) = reinterpret_cast<const copy_t&>(*src);
	};

	uint_fast32_t totalSize = (srcEnd - srcBegin) * sizeof(T);
	while (totalSize > 8)
	switch (totalSize)
	{
	case 1:
		copy(int8_t(), srcBegin, dst);
		break;
	case 2:
		copy(int16_t(), srcBegin, dst);
		break;
	case 3:
		copy(int16_t(), srcBegin, dst);
		copy(int8_t(), srcBegin + 2, dst + 2);
		break;
	case 4:
		copy(int32_t(), srcBegin, dst);
		break;
	case 5:
		copy(int32_t(), srcBegin, dst);
		copy(int8_t(), srcBegin + 4, dst + 4);
		break;
	case 6:
		copy(int32_t(), srcBegin, dst);
		copy(int16_t(), srcBegin + 4, dst + 4);
		break;
	case 7:
		copy(int32_t(), srcBegin, dst);
		copy(int16_t(), srcBegin + 4, dst + 4);
		copy(int8_t(), srcBegin + 6, dst + 6);
		break;
	case 8:
		copy(int64_t(), srcBegin, dst);
		break;
	}
}

static Chances ProcessTest(const std::vector<Card>& playerCards, const std::vector<Card>& opponentCards, const std::vector<Card>& tableCards)
{
	thread_local Card bestPlayerHand[5];
	thread_local Card bestOpponentHand[5];

	thread_local std::vector<Card> playerDeck(playerCards.size() + tableCards.size());
	thread_local std::vector<Card> opponentDeck(opponentCards.size() + tableCards.size());

	std::copy(playerCards.begin(), playerCards.end(), playerDeck.begin());
	std::copy(tableCards.begin(), tableCards.end(), playerDeck.begin() + playerCards.size());
	std::sort(playerDeck.begin(), playerDeck.end());
	GetBestHand(playerDeck, bestPlayerHand);

	std::copy(opponentCards.begin(), opponentCards.end(), opponentDeck.begin());
	std::copy(tableCards.begin(), tableCards.end(), opponentDeck.begin() + opponentCards.size());
	std::sort(opponentDeck.begin(), opponentDeck.end());
	GetBestHand(opponentDeck, bestOpponentHand);

	const auto comparisonResult = CompareHands(bestPlayerHand, bestOpponentHand);

	thread_local Chances chances;
	chances.total = 1;
	chances.winning = (comparisonResult == 1) ? 1 : 0;
	chances.split = (comparisonResult == 0) ? 1 : 0;

	return chances;
}

class ChanceCollector
{
public:
	explicit ChanceCollector(uint_fast32_t numThreads, uint_fast32_t threadBlockSize);
	virtual ~ChanceCollector();

	void Initialize(uint_fast32_t numPlayerCards, uint_fast32_t numOpponentCards, uint_fast32_t numTableCards);
	void JoinAll();
	Chances GetResult() const;

	void AddTest(const std::vector<Card>& playerCards, const std::vector<Card>& opponentCards, const std::vector<Card>& tableCards);

private:
	struct Test
	{
		std::vector<Card> playerCards;
		std::vector<Card> opponentCards;
		std::vector<Card> tableCards;
	};

	struct Signal
	{
		bool condition;
		std::mutex mutex;
		std::condition_variable conditionVariable;

		Signal() : condition(false) {}
		void Wait()
		{
			std::unique_lock<decltype(mutex)> lk(mutex);
			while (!condition)
				conditionVariable.wait(lk, [this]() { return condition; });
		}
		void Clear()
		{
			std::unique_lock<decltype(mutex)> lk(mutex);
			condition = false;
		}
		void Raise()
		{
			std::unique_lock<decltype(mutex)> lk(mutex);
			condition = true;
			lk.unlock();
			conditionVariable.notify_all();
		}
		bool Check()
		{
			std::unique_lock<decltype(mutex)> lk(mutex);
			return condition;
		}
	};

	struct Semaphore
	{
		uint_fast32_t value;
		std::mutex mutex;
		std::condition_variable conditionVariable;

		Semaphore(uint_fast32_t v = 0) : value(v) {}
		void Set(uint_fast32_t v)
		{
			std::unique_lock<decltype(mutex)> lk(mutex);
			value = v;
			if (value > 0)
			{
				lk.unlock();
				conditionVariable.notify_all();
			}
		}
		void Wait()
		{
			std::unique_lock<decltype(mutex)> lk(mutex);
			while (value == 0)
				conditionVariable.wait(lk, [this]() { return value > 0; });
		}
		void Inc()
		{
			std::unique_lock<decltype(mutex)> lk(mutex);
			++value;
			lk.unlock();
			conditionVariable.notify_all();
		}
		void Dec()
		{
			std::unique_lock<decltype(mutex)> lk(mutex);
			if (value > 0)
			{
				--value;
			}
		}
	};

	struct ThreadData
	{
		Chances result;
		std::vector<Test> block;
		int_fast32_t blockFillCount;
		std::mutex resultMutex;
		std::thread thread;
		Signal readyToProcess;
	};

	void WorkerThread(ThreadData& threadData, uint_fast32_t threadBlockSize)
	{
		bool notFinished = true;
		while (notFinished)
		{
			threadData.readyToProcess.Wait();

			Chances results;
			for (const auto& test : threadData.block)
			{
				results += ProcessTest(test.playerCards, test.opponentCards, test.tableCards);
			}

			notFinished = threadData.block.size() == threadBlockSize;

			threadData.blockFillCount = 0;
			threadData.readyToProcess.Clear();
			mReadyToFill.Inc();

			std::lock_guard<decltype(threadData.resultMutex)> lk(threadData.resultMutex);
			threadData.result += results;
		}
	}

	std::deque<ThreadData> mThreadData;
	Semaphore mReadyToFill;
	uint_fast32_t mNumThreads;
	uint_fast32_t mThreadBlockSize;
};

ChanceCollector::ChanceCollector(uint_fast32_t numThreads, uint_fast32_t threadBlockSize)
	: mNumThreads(numThreads)
	, mThreadBlockSize(threadBlockSize)
{}

ChanceCollector::~ChanceCollector()
{}

void ChanceCollector::Initialize(uint_fast32_t numPlayerCards, uint_fast32_t numOpponentCards, uint_fast32_t numTableCards)
{
	mThreadData.resize(mNumThreads);
	for (auto& threadData : mThreadData)
	{
		threadData.block.resize(mThreadBlockSize);
		threadData.blockFillCount = 0;
		for (auto& test : threadData.block)
		{
			test.playerCards.resize(numPlayerCards);
			test.opponentCards.resize(numOpponentCards);
			test.tableCards.resize(numTableCards);
		}
	}

	mReadyToFill.Set(mNumThreads);

	for (auto& threadData : mThreadData)
	{
		threadData.thread = std::thread(&ChanceCollector::WorkerThread, this, std::ref(threadData), mThreadBlockSize);
	}
}

void ChanceCollector::AddTest(const std::vector<Card>& playerCards, const std::vector<Card>& opponentCards, const std::vector<Card>& tableCards)
{
	mReadyToFill.Wait();
	decltype(mThreadData.begin()) maxIt;
	decltype(maxIt->blockFillCount) maxBlock = -1;
	for (auto it = mThreadData.begin(); it != mThreadData.end(); ++it)
	{
		if (!it->readyToProcess.Check())
		{
			if (it->blockFillCount > maxBlock)
			{
				maxBlock = it->blockFillCount;
				maxIt = it;
			}
		}
	}
	assert(maxBlock >= 0);
	assert(maxBlock < mThreadBlockSize);
	auto& test = maxIt->block[maxBlock];
	assert(playerCards.size() == test.playerCards.size());
	assert(opponentCards.size() == test.opponentCards.size());
	assert(tableCards.size() == test.tableCards.size());
	std::copy(playerCards.begin(), playerCards.end(), test.playerCards.begin());
	std::copy(opponentCards.begin(), opponentCards.end(), test.opponentCards.begin());
	std::copy(tableCards.begin(), tableCards.end(), test.tableCards.begin());

	maxIt->blockFillCount++;
	if (maxIt->blockFillCount == maxIt->block.size())
	{
		mReadyToFill.Dec();
		maxIt->readyToProcess.Raise();
	}
}

void ChanceCollector::JoinAll()
{
	for (auto& threadData : mThreadData)
	{
		while (threadData.readyToProcess.Check());
		{
			threadData.block.resize(threadData.blockFillCount);
			threadData.readyToProcess.Raise();
		}
	}

	for (auto& threadData : mThreadData)
	{
		threadData.thread.join();
	}
}

Chances ChanceCollector::GetResult() const
{
	Chances result;
	for (auto& threadData : mThreadData)
		result += threadData.result;
	return result;
}

#define GETCHANCES_MT

Chances GetChances(const std::vector<Card>& playerCards, const std::vector<Card>& opponentCards, const std::vector<Card>& tableCards)
{
	const int32_t numCardsInDeck = static_cast<int32_t>(CardColor::Count) * static_cast<int32_t>(CardValue::Count);

	const int32_t maxPlayerCards = 2;
	const int32_t maxOpponentCards = 2;
	const int32_t maxTableCards = 5;

	const int32_t numPlayerCards = playerCards.size();
	const int32_t numOpponentCards = opponentCards.size();
	const int32_t numTableCards = tableCards.size();

	const int32_t missingPlayerCards = maxPlayerCards - numPlayerCards;
	const int32_t missingOpponentCards = maxOpponentCards - numOpponentCards;
	const int32_t missingTableCards = maxTableCards - numTableCards;

	const int32_t missingTotalCards = missingPlayerCards + missingOpponentCards + missingTableCards;
	const int32_t knownTotalCards = maxPlayerCards + maxOpponentCards + maxTableCards - missingTotalCards;

	Chances chances;
	chances.total
		= Combination(numCardsInDeck - knownTotalCards, missingPlayerCards)
		* Combination(numCardsInDeck - knownTotalCards - missingPlayerCards, missingOpponentCards)
		* Combination(numCardsInDeck - knownTotalCards - missingPlayerCards - missingOpponentCards, missingTableCards);
	chances.winning = 0;
	chances.split = 0;

#ifdef GETCHANCES_MT
	ChanceCollector cc(12, 4096);
	cc.Initialize(maxPlayerCards, maxOpponentCards, maxTableCards);
#endif

	std::vector<Card> innerTableCards(maxTableCards - tableCards.size());
	innerTableCards.insert(innerTableCards.end(), tableCards.begin(), tableCards.end());

	std::vector<Card> playerDeck(maxPlayerCards + maxTableCards);
	std::vector<Card> opponentDeck(maxOpponentCards + maxTableCards);

	Card bestPlayerHand[5];
	Card bestOpponentHand[5];

	std::vector<Card> knownCards;
	knownCards.insert(knownCards.end(), playerCards.begin(), playerCards.end());
	knownCards.insert(knownCards.end(), opponentCards.begin(), opponentCards.end());
	knownCards.insert(knownCards.end(), tableCards.begin(), tableCards.end());
	assert(knownCards.size() == knownTotalCards);

	std::vector<Card> playerOptions;
	for (int32_t i = 0; i < numCardsInDeck; ++i)
	{
		Card c(i);
		if (knownCards.end() == std::find_if(knownCards.begin(), knownCards.end(), [c](Card vc) { return vc == c; }))
		{
			playerOptions.push_back(c);
		}
	}
	//assert(playerOptions.size() == 48);

	std::vector<int32_t> playerPicker(playerOptions.size());
	std::fill(playerPicker.begin(), playerPicker.end() - missingPlayerCards, 0);
	std::fill(playerPicker.end() - missingPlayerCards, playerPicker.end(), 1);

	uintmax_t totalTries = 0;
	do {
		std::vector<Card> playerKnownCards;
		playerKnownCards.insert(playerKnownCards.end(), knownCards.begin(), knownCards.end());
		std::vector<Card> innerPlayerCards;
		innerPlayerCards.insert(innerPlayerCards.end(), playerCards.begin(), playerCards.end());
		for (int32_t k = 0; k < playerOptions.size(); ++k)
		{
			if (playerPicker[k] != 0)
			{
				innerPlayerCards.push_back(playerOptions[k]);
				playerKnownCards.push_back(playerOptions[k]);
			}
		}

		std::vector<Card> opponentOptions;
		for (int32_t i = 0; i < numCardsInDeck; ++i)
		{
			Card c(i);
			if (playerKnownCards.end() == std::find_if(playerKnownCards.begin(), playerKnownCards.end(), [c](Card vc) { return vc == c; }))
			{
				opponentOptions.push_back(c);
			}
		}
		//assert(opponentOptions.size() == 48);

		std::vector<int32_t> opponentPicker(opponentOptions.size());
		std::fill(opponentPicker.begin(), opponentPicker.end() - missingOpponentCards, 0);
		std::fill(opponentPicker.end() - missingOpponentCards, opponentPicker.end(), 1);

		do {
			std::vector<Card> opponentKnownCards;
			opponentKnownCards.insert(opponentKnownCards.end(), playerKnownCards.begin(), playerKnownCards.end());
			std::vector<Card> innerOpponentCards;
			innerOpponentCards.insert(innerOpponentCards.end(), opponentCards.begin(), opponentCards.end());
			for (int32_t k = 0; k < opponentOptions.size(); ++k)
			{
				if (opponentPicker[k] != 0)
				{
					innerOpponentCards.push_back(opponentOptions[k]);
					opponentKnownCards.push_back(opponentOptions[k]);
				}
			}

			std::vector<Card> tableOptions;
			for (int32_t i = 0; i < numCardsInDeck; ++i)
			{
				Card c(i);
				if (opponentKnownCards.end() == std::find_if(opponentKnownCards.begin(), opponentKnownCards.end(), [c](Card vc) { return vc == c; }))
				{
					tableOptions.push_back(c);
				}
			}
			//assert(tableOptions.size() == 48);

			std::vector<int32_t> tablePicker(tableOptions.size());
			std::fill(tablePicker.begin(), tablePicker.end() - missingTableCards, 0);
			std::fill(tablePicker.end() - missingTableCards, tablePicker.end(), 1);

			do {
				int32_t innerTableCardsIndex = 0;
				for (int32_t k = 0; k < tableOptions.size(); ++k)
				{
					if (tablePicker[k] != 0)
					{
						innerTableCards[innerTableCardsIndex++] = tableOptions[k];
					}
				}

#ifdef GETCHANCES_MT
				++totalTries;
				cc.AddTest(innerPlayerCards, innerOpponentCards, innerTableCards);
#else
				std::copy(innerPlayerCards.begin(), innerPlayerCards.end(), playerDeck.begin());
				std::copy(innerTableCards.begin(), innerTableCards.end(), playerDeck.begin() + maxPlayerCards);
				std::sort(playerDeck.begin(), playerDeck.end());
				GetBestHand(playerDeck, bestPlayerHand);

				std::copy(innerOpponentCards.begin(), innerOpponentCards.end(), opponentDeck.begin());
				std::copy(innerTableCards.begin(), innerTableCards.end(), opponentDeck.begin() + maxOpponentCards);
				std::sort(opponentDeck.begin(), opponentDeck.end());
				GetBestHand(opponentDeck, bestOpponentHand);

				++totalTries;
				const auto comparisonResult = CompareHands(bestPlayerHand, bestOpponentHand);
				chances.winning += (comparisonResult == 1) ? 1 : 0;
				chances.split += (comparisonResult == 0) ? 1 : 0;

				//if (comparisonResult == 0)
				//{
				//	printf("Hand type: %13s Hand: %s\n", ToString(GetHandType(&bestPlayerHand[0])).c_str(), ToString(&playerDeck[0], 7).c_str());
				//	printf("Hand type: %13s Hand: %s\n", ToString(GetHandType(&bestOpponentHand[0])).c_str(), ToString(&opponentDeck[0], 7).c_str());
				//	printf("comparisonResult: %d\n", comparisonResult);
				//}
#endif // #ifdef GETCHANCES_MT

			} while (std::next_permutation(tablePicker.begin(), tablePicker.end()));

		} while (std::next_permutation(opponentPicker.begin(), opponentPicker.end()));

	} while (std::next_permutation(playerPicker.begin(), playerPicker.end()));

	printf("totalTries=%llu\n", totalTries);

#ifdef GETCHANCES_MT
	cc.JoinAll();

	return cc.GetResult();
#endif

	return chances;
}

void main()
{
	//PrintSymbol( 50 );
	//srand( (unsigned)time( NULL ) );
	//byte cards[5];
	//HandType ht = THighCard;
	//while( ht != 4 ){
	//	cards[0] = rand() % (rand() % 48 + 1);
	//	for( byte i = 1; i < 5; i++ )
	//		cards[i] = cards[i-1] + 1 + rand() % (48 + i - cards[i-1] - 1);
	//	for( byte i = 0; i < 5; i++ )
	//		PrintSymbol(cards[i]), printf(" ");
	//	ht = GetHandType( cards );
	//	printf( "%i\n", ht );
	//}

	//SaveAllHandsToFile();

	//GameOn();

	//byte tstcards[7];
	//byte bestHand1[5];
	//byte bestHand2[5];
	//tstcards[2] = 4;
	//tstcards[3] = 29;
	//tstcards[4] = 22;
	//tstcards[5] = 23;
	//tstcards[6] = 32;
	//tstcards[0] = 45;
	//tstcards[1] = 50;
	//GetBestHand( tstcards, 7, bestHand1 );
	//tstcards[0] = 51;
	//tstcards[1] = 24;
	//GetBestHand( tstcards, 7, bestHand2 );
	//int res = CompareHands( bestHand1, bestHand2 );

#if 0
	CThreadHelper threadHelper;
	threadHelper.MultithreadedExecute(tetsf, NULL, 0);
	return;
#endif

#if 0
	srand(GetTickCount());

	float fWin, fDraw, ifWin, ifDraw;
	byte hand[2] = { 45, 43 };
	byte table[3] = { 12, 46, 32 };

	int t1 = GetTickCount();
	DecideAfterFlop(hand, table, ifWin, ifDraw);
	printf("Time (inf tries): %d\nfWin: %f\nfDraw: %f\n", GetTickCount() - t1, ifWin, ifDraw);
	t1 = GetTickCount();
	DecideAfterFlop2(hand, table, fWin, fDraw, 100);
	printf("Time (100 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw));
	t1 = GetTickCount();
	DecideAfterFlop2(hand, table, fWin, fDraw, 1000);
	printf("Time (1000 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw));
	t1 = GetTickCount();
	DecideAfterFlop2(hand, table, fWin, fDraw, 10000);
	printf("Time (10000 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw));
	t1 = GetTickCount();
	DecideAfterFlop2(hand, table, fWin, fDraw, 21544);
	printf("Time (21544 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw));
	t1 = GetTickCount();
	DecideAfterFlop2(hand, table, fWin, fDraw, 46416);
	printf("Time (46416 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw));
	t1 = GetTickCount();
	DecideAfterFlop2(hand, table, fWin, fDraw, 100000);
	printf("Time (100000 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw));
	t1 = GetTickCount();
	DecideAfterFlop2(hand, table, fWin, fDraw, 200000);
	printf("Time (200000 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw));

	scanf("%f", fWin);
#endif

#if 1
	//std::vector<Card> cards = {"Ks", "Qs", "Js", "0s", "As", "Ah", "Ad"};
	//std::sort(cards.begin(), cards.end());
	//printf("Cards: %s\n", ToString(&cards[0], 7).c_str());
	//const auto& bestHand = GetBestHand(cards);
	//printf("Hand type: %s Hand: %s\n", ToString(GetHandType(&bestHand[0])).c_str(), ToString(&bestHand[0], 5).c_str());

	Chronometer ch(true);
	const auto& chances = GetChances({"Kh"}, {"Ah"}, {"4d", "5h"});
	printf("Time: %f\n", ch.GetElapsedTimeMs());

	printf("Chances.total=%llu\n",chances.total);
	printf("Chances.winning=%llu\n", chances.winning);
	printf("Chances.split=%llu\n", chances.split);

	printf("You      %6.3f%%\n", 100 * static_cast<double>(chances.winning) / chances.total);
	printf("Opponent %6.3f%%\n", 100 * static_cast<double>(chances.total - chances.winning - chances.split) / chances.total);
	printf("Split    %6.3f%%\n", 100 * static_cast<double>(chances.split) / chances.total);

	//{
	//	auto bv = GetVectorForPermutation<uint8_t>();
	//	uint_fast32_t c = 0;

	//	Chronometer ch(true);
	//	do {
	//		++c;
	//	} while (std::next_permutation(bv.begin(), bv.end()));
	//	printf("Time: %f c=%d\n", ch.GetElapsedTimeMs(), c);
	//}

	//{
	//	auto bv = GetVectorForPermutation<uint16_t>();
	//	uint_fast32_t c = 0;

	//	Chronometer ch(true);
	//	do {
	//		++c;
	//	} while (std::next_permutation(bv.begin(), bv.end()));
	//	printf("Time: %f c=%d\n", ch.GetElapsedTimeMs(), c);
	//}

	//{
	//	auto bv = GetVectorForPermutation<uint32_t>();
	//	uint_fast32_t c = 0;

	//	Chronometer ch(true);
	//	do {
	//		++c;
	//	} while (std::next_permutation(bv.begin(), bv.end()));
	//	printf("Time: %f c=%d\n", ch.GetElapsedTimeMs(), c);
	//}

	//{
	//	auto bv = GetVectorForPermutation<int8_t>();
	//	uint_fast32_t c = 0;

	//	Chronometer ch(true);
	//	do {
	//		++c;
	//	} while (std::next_permutation(bv.begin(), bv.end()));
	//	printf("Time: %f c=%d\n", ch.GetElapsedTimeMs(), c);
	//}

	//{
	//	auto bv = GetVectorForPermutation<int16_t>();
	//	uint_fast32_t c = 0;

	//	Chronometer ch(true);
	//	do {
	//		++c;
	//	} while (std::next_permutation(bv.begin(), bv.end()));
	//	printf("Time: %f c=%d\n", ch.GetElapsedTimeMs(), c);
	//}

	//{
	//	auto bv = GetVectorForPermutation<int32_t>();
	//	uint_fast32_t c = 0;

	//	Chronometer ch(true);
	//	do {
	//		++c;
	//	} while (std::next_permutation(bv.begin(), bv.end()));
	//	printf("Time: %f c=%d\n", ch.GetElapsedTimeMs(), c);
	//}

	char cc;
	scanf("%c", cc);

	return;
#endif
}
