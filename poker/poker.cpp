#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "curses.h"

#include "ThreadHelper.h"
#include "math.h"

typedef unsigned char byte;

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#define ABS(a) ( (a) > 0 ? (a) : -(a) )

typedef enum{
	THighCard = 0,
	TOnePair = 1,
	TTwoPair = 2,
	TThreeOfAKind = 3,
	TStraight = 4,
	TFlush = 5,
	TFullHouse = 6,
	TFourOfAKind = 7,
	TStraightFlush = 8
} HandType;

char* g_den[9] = {"HighCard", "OnePair", "TwoPair", "ThreeOfAKind", "Straight", "Flush", "FullHouse", "FourOfAKind", "StraightFlush"};

bool CardLess( byte c1, byte c2 ){
	return c1 < c2;
}

HandType GetHandType( byte cards[5] ){
	byte color = cards[0] & 3;
	if( color == (cards[1] & 3) && color == (cards[2] & 3) && color == (cards[3] & 3) && color == (cards[4] & 3) ){
		//hand is Flush or StraightFlush
		if( ((cards[4] >> 2) - (cards[0] >> 2) == 4) || ((cards[4] >> 2 == 12) && (cards[3] >> 2 == 3)) )
			return TStraightFlush;
		return TFlush;
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
		return TFourOfAKind;
	if( nmaxofakind == 3 ){
		// hand is FullHouse or ThreeOfAKind
		if( (cards[0] >> 2 == cards[1] >> 2) && (cards[3] >> 2 == cards[4] >> 2) )
			return TFullHouse;
		return TThreeOfAKind;
	}
	if( nmaxofakind == 2 ){
		//hand is TwoPair or OnePair
		byte nequals = 0;
		for( byte i = 1; i < 5; i++ )
			if( cards[i] >> 2 == cards[i-1] >> 2 )
				nequals++;
		if( nequals & 1 )
			return TOnePair;
		return TTwoPair;
	}
	//hand is HighCard or Straight
	if( ((cards[4] >> 2) - (cards[0] >> 2) == 4) || ((cards[4] >> 2 == 12) && (cards[3] >> 2 == 3)) )
		return TStraight;
	return THighCard;
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

	if( ht1 == TStraightFlush || ht1 == TStraight ){
		v1 = hand1[0] >> 2;
		v2 = hand2[0] >> 2;
		if( v1 > v2 )
			return 1;
		if( v1 < v2 )
			return -1;
		return 0;
	}

	if( ht1 == TFourOfAKind || ht1 == TFullHouse ){
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

	if( ht1 == TFlush ){
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

	if( ht1 == TThreeOfAKind ){
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

	if( ht1 == TTwoPair ){
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

	if( ht1 == TOnePair ){
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

	if( ht1 == THighCard ){
		for( byte i = 4; i >= 0; i-- ){
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
						stats[ht]++;
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
				mvprintw(logpos++,0,"Computer wins with a %s.", g_den[GetHandType(bestHand1)] );
				PrintSymbol( sCard, cards[0][0] );
				mvprintw(logpos++,0,"Computer had: %s ", sCard);
				PrintSymbol( sCard, cards[0][1] );
				printw("%s\n", sCard);
				stack[0] += pot;
			}
			else if( res == -1 ){
				mvprintw(logpos++,0,"Human wins with a %s.", g_den[GetHandType(bestHand2)] );
				PrintSymbol( sCard, cards[0][0] );
				mvprintw(logpos++,0,"Computer had: %s ", sCard);
				PrintSymbol( sCard, cards[0][1] );
				printw("%s\n", sCard);
				stack[1] += pot;
			}
			else{
				mvprintw(logpos++,0,"Both players tie a %s.", g_den[GetHandType(bestHand2)] );
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

void main(){
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

	int *a[10];
	for (int i=0; i<10; i++){
		int z = i & 3;
		switch(z){
			case 0:{
				int k = i + 10;
				a[i] = &k;
				   }
				   break;
			case 1:{
				int k = i + 20;
				a[i] = &k;
				   }
				   break;
			case 2:{
				int k = i + 30;
				a[i] = &k;
				   }
				   break;
			case 3:{
				int k = i + 40;
				a[i] = &k;
				   }
				   break;
		}
	}
	for(int i=0; i<10; i++)
	{
		printf("%d ", *a[i]);
	}
	int x; scanf("%d",&x);
	return;

	CThreadHelper threadHelper;
	threadHelper.MultithreadedExecute( tetsf, NULL, 0 );
	return;

	srand( GetTickCount() );

	float fWin, fDraw, ifWin, ifDraw;
	byte hand[2] = { 45, 43 };
	byte table[3] = { 12, 46, 32 };

	int t1 = GetTickCount();
	DecideAfterFlop( hand, table, ifWin, ifDraw );
	printf( "Time (inf tries): %d\nfWin: %f\nfDraw: %f\n", GetTickCount() - t1, ifWin, ifDraw );
	t1 = GetTickCount();
	DecideAfterFlop2( hand, table, fWin, fDraw, 100 );
	printf( "Time (100 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw) );
	t1 = GetTickCount();
	DecideAfterFlop2( hand, table, fWin, fDraw, 1000 );
	printf( "Time (1000 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw) );
	t1 = GetTickCount();
	DecideAfterFlop2( hand, table, fWin, fDraw, 10000 );
	printf( "Time (10000 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw) );
	t1 = GetTickCount();
	DecideAfterFlop2( hand, table, fWin, fDraw, 21544 );
	printf( "Time (21544 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw) );
	t1 = GetTickCount();
	DecideAfterFlop2( hand, table, fWin, fDraw, 46416 );
	printf( "Time (46416 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw) );
	t1 = GetTickCount();
	DecideAfterFlop2( hand, table, fWin, fDraw, 100000 );
	printf( "Time (100000 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw) );
	t1 = GetTickCount();
	DecideAfterFlop2( hand, table, fWin, fDraw, 200000 );
	printf( "Time (200000 tries): %d\nfWin: %f - %f\nfDraw: %f - %f\n", GetTickCount() - t1, fWin, ABS(fWin - ifWin), fDraw, ABS(fDraw - ifDraw) );

	scanf("%f",fWin);
}
