package mobi.omegacentauri.brainlinkfirmwareuploader;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Set;
import java.util.UUID;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.DialogInterface.OnClickListener;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Color;
import android.graphics.drawable.StateListDrawable;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.text.Html;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

public class BrainlinkFirmwareUploader extends Activity {
	private static final String PREF_LAST_DEVICE = "lastDevice";
	private static final String PREF_FIRMWARE = "firmware";
	private BluetoothAdapter btAdapter;
	private TextView message;
	private SharedPreferences options;
	private ArrayAdapter<String> deviceSelectionAdapter;
	private Spinner deviceSpinner;
	private Spinner fwSpinner;
	private ArrayList<BluetoothDevice> devs;
	private static final String PREF_DISCLAIMED_VERSION = "disclaimed";
	private static final String[] firmwares = { "original.hex", "mainFirmware.hex", "57k-roomba.hex" };
	
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		options = PreferenceManager.getDefaultSharedPreferences(this);

		Log.v("BLFW", "OnCreate");
		
		setContentView(R.layout.main);
		
		message = (TextView)findViewById(R.id.message);
		deviceSpinner = (Spinner)findViewById(R.id.device_spinner);
		fwSpinner = (Spinner)findViewById(R.id.firmware_spinner);
		String fw = options.getString(PREF_FIRMWARE, firmwares[1]);
		int fwIndex = 1;
		for (int i = 0 ; i < firmwares.length ; i++) {
			if (firmwares[i].equals(fw)) {
				fwIndex = i;
				break;
			}
		}
		fwSpinner.setSelection(fwIndex);
		fwSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {

			@Override
			public void onItemSelected(AdapterView<?> arg0, View arg1,
					int position, long arg3) {
				options.edit().putString(PREF_FIRMWARE, firmwares[position]);
			}

			@Override
			public void onNothingSelected(AdapterView<?> arg0) {
				// TODO Auto-generated method stub
				
			}
		});
		
		disclaimer();
	}
	
	@Override
	public void onPause() {
		super.onPause();
	}
	
	public void clickedOn(View v) {
		int pos = deviceSpinner.getSelectedItemPosition();
		if (pos < 0) {
			Toast.makeText(this, "Select a device", Toast.LENGTH_LONG).show();
		}
		else {
			new InitializeTask(this, devs.get(pos)).execute(firmwares[fwSpinner.getSelectedItemPosition()]);
		}
	}
	
	public void clickedLicense(View v) {
		AlertDialog.Builder b = new AlertDialog.Builder(this);
		b.setTitle("License for Brainlink Firmware");
		b.setMessage("This work is copyright BirdBrain Technologies (with modifications by Alexander Pruss) and licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. "+
"To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to "+
"Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.");
		b.create().show();
	}
	
	public void clickedFeatures(View v) {
		AlertDialog.Builder b = new AlertDialog.Builder(this);
		b.setTitle("Custom Brainlink Firmware 1.04");
		b.setMessage(Html.fromHtml("1.04:<br/>"+
				"- Fix nasty buffer overflow in 'r' command<br/>" + 
				"- more easy-baud rate settings; now we have: 'u12' (1200 baud), 'u48' (4800), 'u96' (9600), "+
				"'u19' (19200), 'u38' (38400), 'u57' (57600), 'u11' (112500)<br/>"+
				"1.03:<br/>"+
				"- 'J1' command to switch the aux uart to IrDA mode (9600 baud, IrDA 3/16 "+
				"duty cycle pulse shaping); return to default uart settings with 'J0'<br/>"+
				"1.01:<br/>"+
				"- 'Z' command for faster bridge between serial and Bluetooth; no other commands will be accepted afterwards; powercycle to stop<br/>"+				
				"- automatically enter bridge mode at the right baud rate when software attempts to connect to Roomba or Mindflex<br/>"+
				"- easy baud rate setting ('uxx' command)<br/>"+
				"- clear aux buffer when switching baud rate<br/>"));
		b.create().show();
	}
	public void disclaimer() {
		int v;
		try {
			v = getPackageManager().getPackageInfo(getPackageName(), 0).versionCode;
			if (v == options.getInt(PREF_DISCLAIMED_VERSION , 0));
				return;
		} catch (NameNotFoundException e) {
			v = 0;
		}

		AlertDialog.Builder b = new AlertDialog.Builder(this);
		b.setTitle("Disclaimer");
		b.setMessage("USE THIS ONLY AT YOUR OWN RISK.  Omega Centauri Software takes no responsibility for any "+
		"damage to data or hardware (including the Brainlink).  If your Brainlink becomes unresponsive, you can use the links at brainlinksystem.com/firmware-programming to try to reinstall the original firmware.  Do you agree?");
		b.setNegativeButton("Disagree", new OnClickListener() {
			
			@Override
			public void onClick(DialogInterface dialog, int which) {
				BrainlinkFirmwareUploader.this.finish();
			}
		});
		final int v0 = v;
		b.setPositiveButton("Agree",  new OnClickListener() {
			
			@Override
			public void onClick(DialogInterface dialog, int which) {
				options.edit().putInt(PREF_DISCLAIMED_VERSION, v0);
			}
		});
		b.create().show();
	}
	
	@Override
	public void onResume() {
		super.onResume();
		btAdapter = BluetoothAdapter.getDefaultAdapter();
		devs = new ArrayList<BluetoothDevice>();
		devs.addAll(btAdapter.getBondedDevices());
		Collections.sort(devs, new Comparator<BluetoothDevice>(){
			@Override
			public int compare(BluetoothDevice lhs, BluetoothDevice rhs) {
				return String.CASE_INSENSITIVE_ORDER.compare(lhs.getName(), rhs.getName());
			}});
		ArrayList<String> devLabels = new ArrayList<String>();
		for (BluetoothDevice d : devs) 
			devLabels.add(d.getName()+" ("+d.getAddress()+")");
		
		deviceSelectionAdapter = new ArrayAdapter<String>(this, 
				android.R.layout.simple_spinner_item, devLabels);
		deviceSelectionAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		deviceSpinner.setAdapter(deviceSelectionAdapter);
		String lastDev = options.getString(PREF_LAST_DEVICE, "(none)");
		
		for (int i = 0 ; i < devs.size() ; i++) {
			if (devs.get(i).getName().equals("RN42-A308")) {
				deviceSpinner.setSelection(i);
				break;
			}
		}
		
		for (int i = 0 ; i < devs.size() ; i++) {
			if (devs.get(i).getAddress().equals(lastDev))
				deviceSpinner.setSelection(i);
		} 
		
		deviceSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {

			@Override
			public void onItemSelected(AdapterView<?> arg0, View arg1,
					int position, long arg3) {
				options.edit().putString(PREF_LAST_DEVICE, devs.get(position).getAddress()).commit();				
			}

			@Override
			public void onNothingSelected(AdapterView<?> arg0) {
				// TODO Auto-generated method stub
				
			}
		});
		
		if (devs.size() == 0)
			message.setText("Bluetooth turned off or no devices paired.");
	}
	
	class InitializeTask extends AsyncTask<String, String, String>{
		private ProgressDialog progressDialog;
		private BluetoothDevice device;

		public InitializeTask(Context c, BluetoothDevice device) {
			this.device = device;
			progressDialog = new ProgressDialog(c);
			progressDialog.setCancelable(false);
			progressDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			progressDialog.show();
		}
		
		@Override
		public void onProgressUpdate(String... msg) {
			progressDialog.setMessage(msg[0]);
		}
	
		@Override
		protected String doInBackground(String... firmware) {
			BTDataLink link = null;
			Butterfly butterfly = null;
			
			try {
				publishProgress("Connecting");
				link = new BTDataLink(device);
				
				publishProgress("Preparing firmware");
				butterfly = new Butterfly(link, 16384, 128);
				byte[] flash = butterfly.getFlashFromHex(BrainlinkFirmwareUploader.this.getResources().getAssets().open(firmware[0]));
				if (flash == null) 
					throw new Exception("Problem reading .hex file.");

				publishProgress("Uploading firmware");
				if (! butterfly.writeFlash(flash)) 
					throw new Exception("Error uploading firmware.  Brainlink may not function until you upload successfully.");
				
				publishProgress("Verifying firmware");
				byte[] newFlash = butterfly.readFlash();
				if (!Arrays.equals(newFlash, flash)) 
					throw new Exception("Error verifying uploaded firmware.  Brainlink may not function until you upload successfully.");
				
			}
			catch (Exception e) {
				return e.toString();
			}
			finally {
				if (butterfly != null)
					butterfly.stop();
				if (link != null)
					link.stop();
			}
			
			return "Firmware successfully uploaded";
		}

		@Override
		protected void onPostExecute(String message) {
			BrainlinkFirmwareUploader.this.message.setText(message);
			progressDialog.dismiss();
		}
		
	}

	public static void sleep(int ms) {
		try {
			Thread.sleep(ms);
		} catch (InterruptedException e2) {
		}
	}
	
	
}
