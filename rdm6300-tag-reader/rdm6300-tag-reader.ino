#include <SoftwareSerial.h>

#define READ_LED_PIN 13

const int DATA_SIZE = 10;
const int BUFFER_SIZE = 14;
const int DATA_TAG_SIZE = 8;
const int CHECKSUM_SIZE = 2;
const int DATA_VERSION_SIZE = 2;

SoftwareSerial rfid = SoftwareSerial(2, 8);

int buffer_index = 0;
char buffer[BUFFER_SIZE];
unsigned long previous_tag = 0L;

void setup()
{
    Serial.begin(9600);

    pinMode(READ_LED_PIN, OUTPUT);
    digitalWrite(READ_LED_PIN, LOW);

    rfid.begin(9600);
    rfid.listen();

    Serial.println("\nPlace RFID tag near the rdm6300...");
}

void loop()
{
    if (rfid.available() > 0)
    {
        int rfid_value = rfid.read();
        bool should_extract_tag = false;

        if (rfid_value == -1)
        {
            return;
        }

        if (rfid_value == 2)
        {
            buffer_index = 0;
        }
        else if (rfid_value == 3)
        {
            should_extract_tag = true;
        }

        if (buffer_index >= BUFFER_SIZE)
        {
            Serial.println("Error: Buffer overflow detected!");
            return;
        }

        buffer[buffer_index++] = rfid_value;

        if (should_extract_tag == true)
        {
            if (buffer_index == BUFFER_SIZE)
            {
                unsigned rfid_tag = extract_tag_information(true);

                if (previous_tag == rfid_tag)
                {
                    return;
                }
                else
                {
                    previous_tag = extract_tag_information(false);
                }
            }
            else
            {
                buffer_index = 0;
                return;
            }
        }
    }
}

unsigned extract_tag_information(boolean simple)
{
    uint8_t message_head = buffer[0];
    uint8_t message_tail = buffer[13];
    char *message_data = buffer + 1;
    char *message_checksum = buffer + 11;
    char *message_data_tag = message_data + 2;

    if (simple)
    {
        long tag = get_tag_identifier(message_data_tag);
        return tag;
    }

    digitalWrite(READ_LED_PIN, HIGH);

    Serial.println("--------");
    Serial.print("Message-Head: ");
    Serial.println(message_head);
    Serial.println("Message-Data (HEX): ");

    for (int i = 0; i < DATA_VERSION_SIZE; ++i)
    {
        Serial.print(char(message_data[i]));
    }

    Serial.println(" (version)");

    for (int i = 0; i < DATA_TAG_SIZE; ++i)
    {
        Serial.print(char(message_data_tag[i]));
    }

    Serial.println(" (tag)");
    Serial.print("Message-Checksum (HEX): ");

    for (int i = 0; i < CHECKSUM_SIZE; ++i)
    {
        Serial.print(char(message_checksum[i]));
    }

    Serial.println("");
    Serial.print("Message-Tail: ");
    Serial.println(message_tail);
    Serial.println("--");

    long tag = get_tag_identifier(message_data_tag);

    Serial.print("Extracted Tag: ");
    Serial.println(tag);

    long checksum = 0;

    for (int i = 0; i < DATA_SIZE; i += CHECKSUM_SIZE)
    {
        long val = get_value_from_hexstr(message_data + i, CHECKSUM_SIZE);
        checksum ^= val;
    }

    Serial.print("Extracted Checksum (HEX): ");
    Serial.print(checksum, HEX);

    if (checksum == get_value_from_hexstr(message_checksum, CHECKSUM_SIZE))
    {
        Serial.print(" (OK)");
    }
    else
    {
        Serial.print(" (NOT OK)");
    }

    Serial.println("");
    Serial.println("--------");

    digitalWrite(READ_LED_PIN, LOW);

    return tag;
}

long get_tag_identifier(char *message_data_tag)
{
    long tag = get_value_from_hexstr(message_data_tag, DATA_TAG_SIZE);
    return tag;
}

long get_value_from_hexstr(char *str, unsigned int length)
{
    char *copy = (char *)malloc((sizeof(char) * length) + 1);
    memcpy(copy, str, sizeof(char) * length);
    copy[length] = '\0';
    long value = strtol(copy, NULL, 16);
    free(copy);
    return value;
}