#include <Base64.h>
#include <string.h>
// #include <Serial.h>

#define PIN_A 8
#define PIN_B 9
#define ENABLE_PIN 7 // PWM Pin
#define CLOCK_US 200

#define BETWEEN_ZERO 53 // Deadtime

#define TRACKS 2

// Deafualt
char tracks[2][80] = {
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
  "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
};
// Reversed track
char revTrack[41];

// Serial buffer (108 Base64 is a max of 80 ASCII)
const byte numChars = 108;
char receivedChars[numChars];

const int sublen[] = {
	32, 48, 48 };
const int bitlen[] = {
	7, 5, 5 };

unsigned int curTrack = 0;
int dir;

bool play_card = false;

void null_strings(){
  for(int i = 0; i < 2; i++){
    strcpy(tracks[1], "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
  }
}

void setup() {
	pinMode(PIN_A, OUTPUT);
	pinMode(PIN_B, OUTPUT);
	pinMode(ENABLE_PIN, OUTPUT);
  null_strings();
  base64_decode(tracks[0], "JUI0MTI0OTM5OTk5OTk5OTkwXk9NTy9NQVJLXjIwMTJTU1NEREREREREREREREREREREREREREREREREPw==", 84);
  base64_decode(tracks[1], "OzQxMjQ5Mzk5OTk5OTk5OTA9VFJBQ1NTREREREREREREREREREQ/", 52);
	// store reverse track 2 to play later
	storeRevTrack(2);
  Serial.begin(9600);
}

// send a single bit out
void playBit(int sendBit)
{
	dir ^= 1;
	digitalWrite(PIN_A, dir);
	digitalWrite(PIN_B, !dir);
	delayMicroseconds(CLOCK_US);

	if (sendBit)
	{
		dir ^= 1;
		digitalWrite(PIN_A, dir);
		digitalWrite(PIN_B, !dir);
	}
	delayMicroseconds(CLOCK_US);

}

// when reversing
void reverseTrack(int track)
{
	int i = 0;
	track--; // index 0
	dir = 0;

	while (revTrack[i++] != '\0');
	i--;
	while (i--)
		for (int j = bitlen[track]-1; j >= 0; j--)
			playBit((revTrack[i] >> j) & 1);
}

// plays out a full track, calculating CRCs and LRC
void playTrack(int track)
{
	int tmp, crc, lrc = 0;
	track--; // index 0
	dir = 0;

	// PWM High
	digitalWrite(ENABLE_PIN, HIGH);

	// First put out a bunch of leading zeros.
	for (int i = 0; i < 25; i++)
		playBit(0);

	// Write data and compute LRC
	for (int i = 0; tracks[track][i] != '\0'; i++)
	{
		crc = 1;
		tmp = tracks[track][i] - sublen[track];

		for (int j = 0; j < bitlen[track]-1; j++)
		{
			crc ^= tmp & 1;
			lrc ^= (tmp & 1) << j;
			playBit(tmp & 1);
			tmp >>= 1;
		}
		playBit(crc);
	}

	tmp = lrc;
	crc = 1;
	for (int j = 0; j < bitlen[track]-1; j++)
	{
		crc ^= tmp & 1;
		playBit(tmp & 1);
		tmp >>= 1;
	}
	playBit(crc);

	// if track 1, play 2nd track in reverse
	if (track == 0 && false)
	{
		// zeros in between tracks
		for (int i = 0; i < BETWEEN_ZERO; i++)
			playBit(0);

		// send second track in reverse
		reverseTrack(2);
	}

	// finish with 0's
	for (int i = 0; i < 5 * 5; i++)
		playBit(0);

	digitalWrite(PIN_A, LOW);
	digitalWrite(PIN_B, LOW);
	digitalWrite(ENABLE_PIN, LOW);

}



// stores track for reverse usage later
void storeRevTrack(int track)
{
	int i, tmp, crc, lrc = 0;
	track--; // index 0
	dir = 0;

	for (i = 0; tracks[track][i] != '\0'; i++)
	{
		crc = 1;
		tmp = tracks[track][i] - sublen[track];

		for (int j = 0; j < bitlen[track]-1; j++)
		{
			crc ^= tmp & 1;
			lrc ^= (tmp & 1) << j;
			tmp & 1 ?
				(revTrack[i] |= 1 << j) :
				(revTrack[i] &= ~(1 << j));
			tmp >>= 1;
		}
		crc ?
			(revTrack[i] |= 1 << 4) :
			(revTrack[i] &= ~(1 << 4));
	}

	// finish calculating and send last "byte" (LRC)
	tmp = lrc;
	crc = 1;
	for (int j = 0; j < bitlen[track]-1; j++)
	{
		crc ^= tmp & 1;
		tmp & 1 ?
			(revTrack[i] |= 1 << j) :
			(revTrack[i] &= ~(1 << j));
		tmp >>= 1;
	}
	crc ?
		(revTrack[i] |= 1 << 4) :
		(revTrack[i] &= ~(1 << 4));

	i++;
	revTrack[i] = '\0';
}

int readLine() {
  static byte index = 0;
  int ret = 0;
  char endMarker = '\n';
  char rc;
  while (Serial.available() > 0) {
    rc = Serial.read();

    if (rc != endMarker) {
      receivedChars[index] = rc;
      index++;
      if (index >= numChars) {
        index = numChars - 1;
      }
    } else {
      receivedChars[index] = '\0'; // terminate the string
      ret = index;
      index = 0;
    }
  }
  return ret;
}

void loop() {
  delay(250);
  int chars = readLine();
  if(chars > 30){
    Serial.println(chars);
    null_strings();
    base64_decode(tracks[0], receivedChars, chars);
    memset(receivedChars, 0, numChars);
    play_card = true;
  }
  else {
    Serial.print("None ");
    Serial.println(chars);
    memset(receivedChars, 0, numChars);
  }
  Serial.println(play_card);
	if(play_card == true){
  		for(int i = 0; i < 20; i++){
        Serial.print("Playing Track! ");
        Serial.print(i);
        Serial.print("\n");
  			playTrack(1);
  			delay(500);
  		}
     play_card = false;
	}
}
