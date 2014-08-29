package mobi.omegacentauri.brainlinkfirmwareuploader;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.UUID;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.os.Build;
import android.util.Log;

public class BTDataLink extends DataLink {
	BluetoothSocket sock;
	private OutputStream os;
	private InputStream is;

	public BTDataLink(BluetoothDevice dev) throws IOException {
		for (int i = 0 ; i < 3 ; i++) {
			try {
				if (Build.VERSION.SDK_INT >= 10)
					sock = dev.createInsecureRfcommSocketToServiceRecord(UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"));
				else {
					Method m;
					try {
						m = dev.getClass().getMethod("createInsecureRfcommSocket", new Class[] {int.class});
						sock = (BluetoothSocket) m.invoke(dev, 1);
					}
					catch (Exception e) {
						sock = dev.createRfcommSocketToServiceRecord(UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"));
					}
				}
				sock.connect();
				os = sock.getOutputStream();
				is = sock.getInputStream();
				return;
			}
			catch (IOException e) {
				stop();
				if (i + 1 >= 3)
					throw e;
			}
		}
	}


	@Override
	public int getFixedBaud() {
		return 115200;
	}

	@Override
	public byte[] receiveBytes() {
		return null;
	}

	@Override
	public void transmit(byte... data) {
		try {
			os.write(data);
		} catch (IOException e) {
		}
	}

	@Override
	public void clearBuffer() {
		int avail;
		try {
			avail = is.available();
			is.skip(avail);		
		} catch (IOException e) {
		}
	}

	@Override
	public void start(int baud) {
	}

	@Override
	public void stop() {
		if (os != null)
			try {
				os.close();
			} catch (IOException e) {
			}
		os = null;
		if (is != null)
			try {
				is.close();
			} catch (IOException e) {
			}
		is = null;
		if (sock != null)
			try {
				sock.close();
			} catch (IOException e) {
			}
		sock = null;
	}

	@Override
	public boolean readBytes(byte[] data, int timeout) {
		try {
			int pos = 0;
			long t1 = System.currentTimeMillis() + timeout;
	
			while (pos < data.length && System.currentTimeMillis() <= t1) {
				int n = is.available();
	
				if (n > 0) {
					if (pos + n > data.length)
						n = data.length - pos;
					is.read(data, pos, n);
					pos += n;
				}
			}
	
			return pos == data.length;
		} catch(IOException e) {
			return false;
		}
	}
}
