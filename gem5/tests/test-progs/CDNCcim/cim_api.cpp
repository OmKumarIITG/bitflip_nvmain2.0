#include "cim_api.hpp"

void
CimModule::generateCommand(
    const uint8_t op_type, const std::vector<uint8_t> &rows,
    const uint8_t &byte_mask, const uint32_t &bank_mask,
    const uint64_t &column_mask, const uint8_t &dest)
{
    assert((rows.size() >= 2) || (rows.size() <= 8));

    CommandEncode c1(this->commandWriteAddress);

    c1.operation_type = op_type;
    c1.byte_mask = byte_mask;
    c1.bank_mask = bank_mask;
    c1.column_mask = column_mask;
    c1.dest = dest;
    for (size_t i = 0; i < rows.size(); i++)
    {
        c1.row_number[i] = rows[i];
        c1.operation_flag_mask |= (1u << i);
    }

    c1.print();
    c1.issue();
}

void
CimModule::AND(
    const std::vector<uint8_t> &rows, const uint8_t &byte_mask,
    const uint32_t &bank_mask, const uint64_t &column_mask,
    const uint8_t &dest)
{
    generateCommand(0, rows, byte_mask, bank_mask, column_mask, dest);
}

void
CimModule::OR(
    const std::vector<uint8_t> &rows, const uint8_t &byte_mask,
    const uint32_t &bank_mask, const uint64_t &column_mask,
    const uint8_t &dest)
{
    generateCommand(1, rows, byte_mask, bank_mask, column_mask, dest);
}

void
CimModule::XOR(
    const std::vector<uint8_t> &rows, const uint8_t &byte_mask,
    const uint32_t &bank_mask, const uint64_t &column_mask,
    const uint8_t &dest)
{
    generateCommand(2, rows, byte_mask, bank_mask, column_mask, dest);
}

void
CimModule::COPY(
    const uint16_t &dest, const uint16_t &src, const uint8_t &rotate_left,
    const uint8_t &byte_mask, const uint32_t &bank_mask,
    const uint64_t &column_mask)
{
    CommandEncode c1(this->commandWriteAddress);

    c1.operation_type = 3;
    c1.byte_mask = byte_mask;
    c1.bank_mask = bank_mask;
    c1.column_mask = column_mask;
    c1.dest = dest;
    c1.row_number[0] = src;
    c1.operation_flag_mask = rotate_left;

    c1.print();
    c1.issue();
}

void
CimModule::NOT_COND(
    const uint16_t &dest, const uint16_t &src, const bool &always_NOT,
    const bool &NOT_if_zero, const uint8_t &byte_mask,
    const uint32_t &bank_mask, const uint64_t &column_mask)
{
    CommandEncode c1(this->commandWriteAddress);

    c1.operation_type = 4;
    c1.byte_mask = byte_mask;
    c1.bank_mask = bank_mask;
    c1.column_mask = column_mask;
    c1.dest = dest;
    c1.row_number[0] = src;
    c1.operation_flag_mask += (always_NOT) ? 2 : 0;
    c1.operation_flag_mask += (NOT_if_zero) ? 1 : 0;

    c1.print();
    c1.issue();
}

void
CimModule::copy_to_cim(
    const uint16_t &row, void *cpu_array, size_t size_in_byte)
{
    std::memcpy(
        (void *)(readWriteAddress + (row * DEFAULT_ROW_SIZE_BYTE >> 3)),
        cpu_array, size_in_byte);
}

void
CimModule::copy_to_cpu(
    void *cpu_array, const uint16_t &row, size_t size_in_byte)
{
    std::memcpy(
        cpu_array,
        (void *)(readWriteAddress + (row * DEFAULT_ROW_SIZE_BYTE >> 3)),
        size_in_byte);
}

void
CimModule::CommandEncode::print()
{
    return;
    printf("-------\n** Printing command: \n");
    printf(
        "type: %02x, flag: %02x, byte_mask: %02x\n", operation_type,
        operation_flag_mask, byte_mask);
    printf("bank_mask: %08x , column_mask: %016lx\n", bank_mask, column_mask);
    printf("row: %04x \n", row_number[0]);
    printf("row: %04x \n", row_number[1]);
    printf("row: %04x \n", row_number[2]);
    printf("row: %04x \n", row_number[3]);
    printf("row: %04x \n", row_number[4]);
    printf("row: %04x \n", row_number[5]);
    printf("row: %04x \n", row_number[6]);
    printf("row: %04x \n", row_number[7]);
    printf("dest: %04x \n------\n", dest);
}

void
CimModule::CommandEncode::issue()
{
    uint8_t row_counter = 0;
    for (auto row : row_number)
    {
        if (row < 256)
            row_counter++;
    }

    // Check for short Command
    if ((row_counter <= 4) && (bank_mask == 0xffffffffu)
        && (column_mask == 0xfffffffffffffffful))
    {
        uint64_t command_to_send = 0;
        //
        command_to_send |= ((uint64_t)(operation_type | 0x80u)) << (8 * 7);
        command_to_send |= ((uint64_t)operation_flag_mask) << (8 * 6);
        command_to_send |= ((uint64_t)byte_mask) << (8 * 4);
        //
        if ((operation_type % 0x80) < 3) // and or xor:
        {
            command_to_send |= ((uint64_t)dest) << (8 * 5);
            for (size_t i = 0; i < row_counter; i++)
            {
                command_to_send |= ((uint64_t)(row_number[i] & 0xffu))
                                   << (8 * i);
            }
        }
        else
        {
            command_to_send |= ((uint64_t)dest) << (8 * 2);
            command_to_send |= ((uint64_t)(row_number[0] & 0xffffu));
        }
        // volatile uint64_t *command_address = (uint64_t *)commandAddress;
        *commandAddress = command_to_send;
    }
    else
    {
        uint64_t command_to_send[3] { 0 };
        command_to_send[0] |= ((uint64_t)operation_type) << (8 * 7);
        command_to_send[0] |= ((uint64_t)operation_flag_mask) << (8 * 6);
        command_to_send[0] |= ((uint64_t)byte_mask) << (8 * 4);
        command_to_send[0] |= ((uint64_t)bank_mask);
        //
        command_to_send[1] = column_mask;
        //
        if ((operation_type % 0x80) < 3) // and or xor:
        {
            command_to_send[0] |= ((uint64_t)dest) << (8 * 5);
            for (size_t i = 0; i < row_counter; i++)
            {
                command_to_send[2] |= ((uint64_t)(row_number[i] & 0xffu))
                                      << (8 * i);
            }
        }
        else
        {
            command_to_send[2] |= ((uint64_t)dest) << (8 * 2);
            command_to_send[2] |= ((uint64_t)(row_number[0] & 0xffffu));
        }

        // volatile uint64_t *command_address = (uint64_t *)commandAddress;
        commandAddress[0] = command_to_send[0];
        commandAddress[1] = command_to_send[1];
        commandAddress[2] = command_to_send[2];
    }
}
