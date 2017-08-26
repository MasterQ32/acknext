#include <acknext.h>
#include <zlib.h>
#include <stdlib.h>
#include <string.h>

void debug_tools();

typedef struct Hf2Header
{
// 	File ID	0	4	string	Should be “HF2” (null terminated)
	char magic[4];
//	Version no.	4	2	unsigned short	Should be 0.
	uint16_t version;
//	Width	6	4	unsigned long	Width of map in pixels.
	uint32_t width;
//	Height	10	4	unsigned long	Height of map in pixels.
	uint32_t height;
//	Tile size	14	2	unsigned short	Size of internal map tiles (8→65535). Default is 256.
	uint16_t tileSize;
//	Vert. precis.	16	4	float	Precision of vertical scale, in metres. Must be greater than zero. Default is 0.01.
	float verticalPrecision;
//	Horiz. scale	20	4	float	Horizontal pixel spacing, in metres. Must be greater than zero. Default is 1.
	float horizontalScale;
//	Ext. header length	24	4	unsigned long	Length of extended header, in bytes. Zero is default.
	uint32_t extHeaderLen;
} __attribute__((packed)) Hf2Header;

typedef struct Hf2ExtHeader
{
	char type[4];    // Null-terminated string describing type of block. Typically “txt”, “xml”, or “bin”. Not case-sensitive.
	char name[16];   // Null-terminated string containing name of block. If null, the block should be read and stored but not interpreted.
	uint32_t length; // Number of bytes in the block, immediately following block length. May be zero.
	// virtual void* // User defined data. Length is given by 'Block length'. May be text, XML, binary, whatever.
} __attribute__((packed)) Hf2ExtHeader;

typedef struct Hf2TileHeader
{
	float verticalScale;  // The vertical scaling of the data in the tile.
	float verticalOffset; // The vertical offset of the data in the tile.
} __attribute__((packed)) Hf2TileHeader;

typedef struct Hf2LineHeader
{
	uint8_t depth; // The byte-depth used for difference encoding in the line. May be 1, 2 or 4.
	int32_t first; // The starting value of the line, encoded as described below.
} __attribute__((packed)) Hf2LineHeader;

void gamemain()
{
	view_create((RENDERCALL)render_scene_with_camera, camera);
	task_defer((ENTRYPOINT)debug_tools, NULL);
	event_attach(on_escape, (EVENTHANDLER)engine_shutdown);
	filesys_addResource("/home/felix/projects/acknext/project-z/resources/", "/");

	BLOB * packed = blob_load("/terrain/GrassyMountains_HF.hfz");
	engine_log("Packed size: %lu", packed->size);

	BLOB * unpacked = blob_inflate(packed);
	engine_log("Unpacked size: %lu", unpacked->size);

	Hf2Header * header = unpacked->data;

	engine_log("HF2 Header:");
	engine_log("\tID             \t%s", header->magic);
	engine_log("\tVersion        \t%d", header->version);
	engine_log("\tWidth          \t%d", header->width);
	engine_log("\tHeight         \t%d", header->height);
	engine_log("\tTile Size      \t%d", header->tileSize);
	engine_log("\tVert. Precision\t%f", header->verticalPrecision);
	engine_log("\tHoriz. Scale   \t%f", header->horizontalScale);
	engine_log("\tExt. Header Len\t%d", header->extHeaderLen);

	uint32_t numBytes = header->extHeaderLen;
	char * ptr = unpacked->data;
	ptr += sizeof(Hf2Header);
	while(numBytes > 0)
	{
		Hf2ExtHeader * ehdr = ptr;
		ptr += sizeof(Hf2ExtHeader);
		void * data = ptr;
		ptr += ehdr->length;

		engine_log("Extended Header:");
		engine_log("\tType: %s", ehdr->type);
		engine_log("\tName: %s", ehdr->name);
		engine_log("\tSize: %lu", ehdr->length);

		numBytes -= sizeof(Hf2ExtHeader);
		numBytes -= ehdr->length;
	}

	BUFFER * vbuffer = buffer_create(VERTEXBUFFER);
	buffer_set(vbuffer, sizeof(VERTEX) * header->width * header->height, NULL);

	VERTEX * superarray = buffer_map(vbuffer, WRITEONLY);

	int tileCountW = (header->width + header->tileSize - 1) / header->tileSize;
	int tileCountH = (header->height + header->tileSize - 1) / header->tileSize;

	for(int ty = 0; ty < tileCountH; ty++) // south-to-north
	{
		for(int tx = 0; tx < tileCountW; tx++) // west-to-east
		{
			Hf2TileHeader * thdr = ptr;
			ptr += sizeof(Hf2TileHeader);

			int countX = minv(header->tileSize * (tx + 1), header->width) - header->tileSize * tx;
			int countY = minv(header->tileSize * (ty + 1), header->width) - header->tileSize * ty;

			engine_log("read tile (%d,%d) with size (%d, %d)", tx, ty, countX, countY);

			for(int l = 0; l < header->tileSize; l++) // south-to-north
			{
				Hf2LineHeader * lhdr = ptr;
				ptr += sizeof(Hf2LineHeader);

				int32_t lastValue = 0;
				for(int i = 0; i < header->tileSize; i++) // west-to-east
				{
					uint32_t fileValue = 0;
					if(i == 0) {
						fileValue = lhdr->first;
					} else {
						switch(lhdr->depth) {
							case 1:
								fileValue = *((int8_t*)ptr);
								break;
							case 2:
								fileValue = *((int16_t*)ptr);
								break;
							case 4:
								fileValue = *((int32_t*)ptr);
								break;
							default: abort();
						}
						ptr += lhdr->depth;
					}

					int32_t currentValue = fileValue + lastValue;

					float h = (float)currentValue * thdr->verticalScale + thdr->verticalOffset;
					int x = header->tileSize * tx + i;
					int y = header->tileSize * ty + l;
					if(x >= header->width || y >= header->height) abort();

					superarray[y * header->width + x] = (VERTEX) {
						.position = (VECTOR){ x, h, y },
					};

					lastValue = currentValue;
				}
			}
		}
	}
	buffer_unmap(vbuffer);

	BUFFER * ibuffer = buffer_create(INDEXBUFFER);
	buffer_set(ibuffer, 6 * sizeof(INDEX) * 255 * 255, NULL);
	INDEX * indices = buffer_map(ibuffer, WRITEONLY);

	int stride = 256;
	for(int y = 0; y < 255; y++)
	{
		for(int x = 0; x < 255; x++)
		{
			int * face = &indices[6 * (y * 255 + x)];
			int base = stride * y + x;
			face[0] = base + 0;
			face[1] = base + 1;
			face[2] = base + stride;
			face[3] = base + 1;
			face[4] = base + stride;
			face[5] = base + stride + 1;

		}
	}

	buffer_unmap(ibuffer);

	MESH mesh;
	mesh.indexBuffer = ibuffer;
	mesh.vertexBuffer = vbuffer;
	mesh.material = NULL;

	MODEL model;

	/*
	SHADER * shdTerrain = shader_create();
	shader_addFileSource(shdTerrain, VERTEXSHADER, "shaders/terrain.vert");
	shader_addFileSource(shdTerrain, FRAGMENTSHADER, "shaders/terrain.frag");
	shader_addFileSource(shdTerrain, FRAGMENTSHADER, "/builtin/shaders/lighting.glsl");
	shader_addFileSource(shdTerrain, FRAGMENTSHADER, "/builtin/shaders/gamma.glsl");
	shader_addFileSource(shdTerrain, FRAGMENTSHADER, "/builtin/shaders/ackpbr.glsl");
	shader_link(shdTerrain);
	*/

}

int main(int argc, char *argv[])
{
	return engine_main(gamemain, argc, argv);
}